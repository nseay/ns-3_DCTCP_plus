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

std::stringstream filePlotQueue1;
std::stringstream filePlotQueue2;
std::ofstream rxS1R1Throughput;
std::ofstream rxS2R2Throughput;
std::ofstream rxS3R1Throughput;
std::ofstream fairnessIndex;
std::ofstream t1QueueLength;
std::ofstream t2QueueLength;


void
PrintProgress (Time interval)
{
  std::cout << "Progress to " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << " seconds simulation time" << std::endl;
  Simulator::Schedule (interval, &PrintProgress, interval);
}


void
PrintThroughput (Time measurementWindow)
{
  for (std::size_t i = 0; i < 10; i++)
    {
      rxS1R1Throughput << measurementWindow.GetSeconds () << "s " << i << " " << (rxS1R1Bytes[i] * 8) / (measurementWindow.GetSeconds ()) / 1e6 << std::endl;
    }
  for (std::size_t i = 0; i < 20; i++)
    {
      rxS2R2Throughput << Simulator::Now ().GetSeconds () << "s " << i << " " << (rxS2R2Bytes[i] * 8) / (measurementWindow.GetSeconds ()) / 1e6 << std::endl;
    }
  for (std::size_t i = 0; i < 10; i++)
    {
      rxS3R1Throughput << Simulator::Now ().GetSeconds () << "s " << i << " " << (rxS3R1Bytes[i] * 8) / (measurementWindow.GetSeconds ()) / 1e6 << std::endl;
    }
}


void
CheckT1QueueSize (Ptr<QueueDisc> queue)
{
  // 1500 byte packets
  uint32_t qSize = queue->GetNPackets ();
  Time backlog = Seconds (static_cast<double> (qSize * 1500 * 8) / 1e10); // 10 Gb/s
  // report size in units of packets and ms
  t1QueueLength << std::fixed << std::setprecision (2) << Simulator::Now ().GetSeconds () << " " << qSize << " " << backlog.GetMicroSeconds () << std::endl;
  // check queue size every 1/100 of a second
  Simulator::Schedule (MilliSeconds (10), &CheckT1QueueSize, queue);
}

void
CheckT2QueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetNPackets ();
  Time backlog = Seconds (static_cast<double> (qSize * 1500 * 8) / 1e9); // 1 Gb/s
  // report size in units of packets and ms
  t2QueueLength << std::fixed << std::setprecision (2) << Simulator::Now ().GetSeconds () << " " << qSize << " " << backlog.GetMicroSeconds () << std::endl;
  // check queue size every 1/100 of a second
  Simulator::Schedule (MilliSeconds (10), &CheckT2QueueSize, queue);
}

int main (int argc, char *argv[])
{
  std::string outputFilePath = "./outputs";
  std::string tcpTypeId = "TcpDctcp";
  int numFlows = 9;
  bool enableSwitchEcn = true;
  Time progressInterval = MicroSeconds (100);

  CommandLine cmd (__FILE__);
  cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
  cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
  cmd.AddValue ("numFlows", "set the number of flows sent to aggregator", numFlows);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Time startTime = Seconds (0);
  Time stopTime = Seconds (60);


  /******** Create Nodes ********/
  NS_LOG_DEBUG("Creating Nodes...");
  Ptr<Node> aggregator = CreateObject<Node> ();
  Ptr<Node> S1 = CreateObject<Node> ();

  int numIntermediateSwitches = 3;
  NodeContainer switches234;
  switches234.Create(numIntermediateSwitches);
  
  NodeContainer senders;
  senders.Create(numFlows);
  
  PointToPointHelper link;
  pointToPointSenders.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointToPointSenders.SetChannelAttribute ("Delay", StringValue ("50us"));

  
  /******** Create Channels and Net Devices ********/
  NS_LOG_DEBUG("Creating Channels and Net Devices...");
  // Total of 13 links.
  // Layer 1
  NetDeviceContainer S1ToA = link.Install(S1, aggregator);

  // Layer 2
  std::vector<NetDeviceContainer> intermediateSwitchesToS1;
  intermediateSwitchesToS1.reserve(numIntermediateSwitches)

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

  // TODO: Update segsize later
  uint32_t tcpSegmentSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // Set default parameters for RED queue disc
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
  // ARED may be used but the queueing delays will increase; it is disabled
  // here because the SIGCOMM paper did not mention it
  // Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
  // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
  // DCTCP+ used switches with 128KB of buffer
  // If every packet is 1500 bytes, ~85 packets can be stored in 128 KB
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("85p")));
  // DCTCP tracks instantaneous queue length only; so set QW = 1
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (20));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (60));


  /******** Install Internet Stack ********/
  NS_LOG_DEBUG("Installing Internet Stack...");
  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tchRed10;
  // // MinTh = 50, MaxTh = 150 recommended in ACM SIGCOMM 2010 DCTCP Paper
  // // This yields a target (MinTh) queue depth of 60us at 10 Gb/s
  // tchRed10.SetRootQueueDisc ("ns3::RedQueueDisc",
  //                            "LinkBandwidth", StringValue ("10Gbps"),
  //                            "LinkDelay", StringValue ("10us"),
  //                            "MinTh", DoubleValue (50),
  //                            "MaxTh", DoubleValue (150));
  // QueueDiscContainer queueDiscs1 = tchRed10.Install (T1T2);

  TrafficControlHelper tchRed;
  // MinTh = 20, MaxTh = 60 recommended in ACM SIGCOMM 2010 DCTCP Paper
  // This yields a target queue depth of 250us at 1 Gb/s
  tchRed.SetRootQueueDisc ("ns3::RedQueueDisc",
                            "LinkBandwidth", StringValue ("1Gbps"),
                            "LinkDelay", StringValue ("50us"),
                            "MinTh", DoubleValue (20),
                            "MaxTh", DoubleValue (60));
  QueueDiscContainer queueDiscs = tchRed.Install (R1T2.Get (1));
  
  tchRed.Install (S1ToA);
  
  for (std::size_t i = 0; i < numIntermediateSwitches; i++)
    {
      tchRed.Install (intermediateSwitchesToS1[i].Get (1));
    }
  for (std::size_t i = 0; i < numFlows; i++)
    {
      tchRed.Install (sendersToIntermediateSwitches[i].Get (1));
    }

  Ipv4InterfaceContainer ipS1ToA = address.Assign (S1ToA);
  std::vector<Ipv4InterfaceContainer> ipIntermediatesToS1;
  ipIntermediatesToS1.reserve(numIntermediateSwitches);
  std::vector<Ipv4InterfaceContainer> ipSendersToIntermediates;
  ipSendersToIntermediates.reserve(numFlows);

  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");
  
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
  aggregatorApp.Start (Seconds (0.0));
  aggregatorApp.Stop (Seconds ((double)time));

  // Sender Applications
  std::vector<BulkSendHelper> bulkSenders;
  bulkSenders.reserve(numFlows);
  std::vector<ApplicationContainer> senderApps;
  senderApps.reserve(numFlows);
  for (std::size_t i = 0; i < numFlows; i++) {
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", aggregatorAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));
    bulkSenders.push_back(ftp);
    ApplicationContainer senderApp = ftp.Install (senders[i]);
    senderApp.Start (Seconds (0.0));
    senderApp.Stop (Seconds ((double)time));
    senderApps.push_back(senderApp);
  }

  /*********** PROGRESS STOPS HERE ***********/


  rxS1R1Throughput.open ("dctcp-example-s1-r1-throughput.dat", std::ios::out);
  rxS1R1Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
  rxS2R2Throughput.open ("dctcp-example-s2-r2-throughput.dat", std::ios::out);
  rxS2R2Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
  rxS3R1Throughput.open ("dctcp-example-s3-r1-throughput.dat", std::ios::out);
  rxS3R1Throughput << "#Time(s) flow thruput(Mb/s)" << std::endl;
  fairnessIndex.open ("dctcp-example-fairness.dat", std::ios::out);
  t1QueueLength.open ("dctcp-example-t1-length.dat", std::ios::out);
  t1QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;
  t2QueueLength.open ("dctcp-example-t2-length.dat", std::ios::out);
  t2QueueLength << "#Time(s) qlen(pkts) qlen(us)" << std::endl;
  for (std::size_t i = 0; i < 10; i++)
    {
      s1r1Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS1R1Sink, i));
    }
  for (std::size_t i = 0; i < 20; i++)
    {
      r2Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS2R2Sink, i));
    }
  for (std::size_t i = 0; i < 10; i++)
    {
      s3r1Sinks[i]->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceS3R1Sink, i));
    }
  Simulator::Schedule (flowStartupWindow + convergenceTime, &InitializeCounters);
  Simulator::Schedule (flowStartupWindow + convergenceTime + measurementWindow, &PrintThroughput, measurementWindow);
  Simulator::Schedule (flowStartupWindow + convergenceTime + measurementWindow, &PrintFairness, measurementWindow);
  Simulator::Schedule (progressInterval, &PrintProgress, progressInterval);
  Simulator::Schedule (flowStartupWindow + convergenceTime, &CheckT1QueueSize, queueDiscs1.Get (0));
  Simulator::Schedule (flowStartupWindow + convergenceTime, &CheckT2QueueSize, queueDiscs2.Get (0));
  Simulator::Stop (stopTime + TimeStep (1));

  Simulator::Run ();

  rxS1R1Throughput.close ();
  rxS2R2Throughput.close ();
  rxS3R1Throughput.close ();
  fairnessIndex.close ();
  t1QueueLength.close ();
  t2QueueLength.close ();
  Simulator::Destroy ();
  return 0;
}
