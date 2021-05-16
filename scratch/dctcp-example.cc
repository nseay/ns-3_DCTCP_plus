/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Topology (FILL IN)
 *
*/

#include <iostream>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DCTCPExample");
std::stringstream filePlotQueue1;
std::stringstream filePlotQueue2;
std::ofstream completionTimesStream;

uint64_t aggregatorBytes;
void TraceAggregator (std::size_t index, Ptr<const Packet> p, const Address& a)
{
  aggregatorBytes += p->GetSize ();
  if (aggregatorBytes >= 1000 * 1024)
  {
    completionTimesStream << aggregatorBytes << " : " << Simulator::Now ().GetMilliSeconds () << std::endl;
  }
}

int main (int argc, char *argv[])
{
  std::string outputFilePath = "./outputs";
  std::string tcpTypeId = "TcpDctcp";
  size_t numFlows = 9;
  bool enableSwitchEcn = true;
  Time progressInterval = MicroSeconds (100);

  CommandLine cmd (__FILE__);
  cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
  cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
  cmd.AddValue ("numFlows", "set the number of flows sent to aggregator", numFlows);
  cmd.Parse (argc, argv);
  LogComponentEnable("DCTCPExample", LOG_LEVEL_DEBUG);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Time startTime = Seconds (0);
  Time stopTime = Seconds (2);


  /******** Create Nodes ********/
  NS_LOG_DEBUG("Creating Nodes...");
  NS_LOG_DEBUG(std::to_string(numFlows) + " flows");
  Ptr<Node> aggregator = CreateObject<Node> ();
  Ptr<Node> S1 = CreateObject<Node> ();

  size_t numIntermediateSwitches = 3;
  NodeContainer switches234;
  switches234.Create(numIntermediateSwitches);
  
  /* Need to decide whether we want numFlows servers or 9 servers*/
  NodeContainer senders;
  senders.Create(numFlows);
  
  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link.SetChannelAttribute ("Delay", StringValue ("50us"));

  
  /******** Create Channels and Net Devices ********/
  NS_LOG_DEBUG("Creating Channels and Net Devices...");
  // Total of 13 links.
  // Layer 1
  NetDeviceContainer S1ToA = link.Install(S1, aggregator);

  // Layer 2
  std::vector<NetDeviceContainer> intermediateSwitchesToS1;
  intermediateSwitchesToS1.reserve(numIntermediateSwitches);

  for (std::size_t i = 0; i < numIntermediateSwitches; i++)
    {
      Ptr<Node> intermediate = switches234.Get (i);
      intermediateSwitchesToS1.push_back (
        link.Install (S1, intermediate)
      );
    }

  // Layer 3
  std::vector<NetDeviceContainer> sendersToIntermediateSwitches;
  sendersToIntermediateSwitches.reserve (numFlows);

  for (std::size_t i = 0; i < numFlows; i++)
    {
      Ptr<Node> sender = senders.Get (i);
      sendersToIntermediateSwitches.push_back (
        link.Install (sender, switches234.Get (i % numIntermediateSwitches))
      );
    }
  
  /******** Set DCTCP defaults ********/
  NS_LOG_DEBUG("Setting TCP/DCTCP Defaults...");

  uint32_t tcpSegmentSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (Seconds (0.01)));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // Set default parameters for RED queue disc
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
  // ARED may be used but the queueing delays will increase; it is disabled
  // here because the SIGCOMM paper did not mention it
  // Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
  // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
  // DCTCP+ paper used switches with 128KB of buffer for all tests
  // If every packet is 1500 bytes, ~85 packets can be stored in 128 KB
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("85p")));
  // DCTCP tracks instantaneous queue length only; so set QW = 1
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  
  // Same as K for DCTCP+
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (20));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (20));


  /******** Install Internet Stack ********/
  NS_LOG_DEBUG("Installing Internet Stack...");
  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tchRed;
  // MinTh = 20, MaxTh = 60 recommended in ACM SIGCOMM 2010 DCTCP Paper
  // This yields a target queue depth of 250us at 1 Gb/s
  tchRed.SetRootQueueDisc ("ns3::RedQueueDisc",
                            "LinkBandwidth", StringValue ("1Gbps"),
                            "LinkDelay", StringValue ("50us"),
                            "MinTh", DoubleValue (20),
                            "MaxTh", DoubleValue (20));
  
  QueueDiscContainer queueDiscs = tchRed.Install (S1ToA);
  for (std::size_t i = 0; i < numIntermediateSwitches; i++)
    {
      tchRed.Install (intermediateSwitchesToS1[i]);
    }
  for (std::size_t i = 0; i < numFlows; i++)
    {
      tchRed.Install (sendersToIntermediateSwitches[i]);
    }

  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");
 
  Ipv4InterfaceContainer ipS1ToA = address.Assign (S1ToA);
  std::vector<Ipv4InterfaceContainer> ipIntermediatesToS1;
  ipIntermediatesToS1.reserve(numIntermediateSwitches);
  std::vector<Ipv4InterfaceContainer> ipSendersToIntermediates;
  ipSendersToIntermediates.reserve(numFlows);
  
  for (std::size_t i = 0; i < numIntermediateSwitches; i++)
    {
      address.NewNetwork ();
      ipIntermediatesToS1.push_back (address.Assign (intermediateSwitchesToS1[i]));
    }

  for (std::size_t i = 0; i < numFlows; i++)
    {
      address.NewNetwork ();
      ipSendersToIntermediates.push_back (address.Assign (sendersToIntermediateSwitches[i]));
    }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /******** Setting up the Application ********/  
  NS_LOG_DEBUG("Setting up Application...");

  // Each sender sends to aggregator
  // aggregator ip
  uint16_t aggregatorPort = 5000;
  AddressValue aggregatorAddress (InetSocketAddress (
    ipS1ToA.GetAddress(1), aggregatorPort));
  
  PacketSinkHelper aggHelper ("ns3::TcpSocketFactory",
                                   aggregatorAddress.Get());
  aggHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ApplicationContainer aggregatorApp = aggHelper.Install (aggregator);
  aggregatorApp.Start (startTime);
  aggregatorApp.Stop (stopTime);
  
  Ptr<PacketSink> aggSink = aggregatorApp.Get (0)->GetObject<PacketSink> ();
  // Sender Applications
  std::vector<BulkSendHelper> bulkSenders;
  bulkSenders.reserve (numFlows);
  std::vector<ApplicationContainer> senderApps;
  senderApps.reserve (numFlows);
  uint64_t onemb = 1024 * 1024;
  for (std::size_t i = 0; i < numFlows; i++) {
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", aggregatorAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));
    uint64_t maxBytes = i < onemb % numFlows ? onemb / numFlows + 1 : onemb / numFlows;
    ftp.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    bulkSenders.push_back (ftp);
    ApplicationContainer senderApp = ftp.Install (senders.Get (i));
    //senderApp.Get (0)->SetMaxBytes (onemb / numFlows);
    senderApp.Start (startTime);
    senderApp.Stop (stopTime);
    senderApps.push_back (senderApp);
  }

  /*********** PROGRESS STOPS HERE ***********/
  NS_LOG_DEBUG("Opening output file(s)...");

  
  completionTimesStream.open ("dctcp-completion-times-" + std::to_string(numFlows) + "flows.txt", std::ios::out);
  aggSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceAggregator, 0));
  
  NS_LOG_DEBUG("Starting simulation...");
  Simulator::Stop (stopTime);

  Simulator::Run ();

  completionTimesStream.close ();
  Simulator::Destroy ();
  return 0;
}
