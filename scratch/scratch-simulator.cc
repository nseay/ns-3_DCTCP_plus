/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Topology (FILL IN)
 */
#include <iostream>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DCTCP-PlusExperiment");
const uint64_t ONE_MB =  1024 * 1024;
std::stringstream filePlotQueue1;
std::stringstream filePlotQueue2;
std::ofstream completionTimesStream;
int printLastXBytesReceived = 0;
size_t numFlows = 9;
Time firstFlowStart = Seconds(4); // high enough to ensure it's overwritten
uint finishTime = 0;
uint randomStartWindow = 10; // µs
Time traceStart = MicroSeconds (double(randomStartWindow));

uint64_t aggregatorBytes;

static void QueueOccupancyTracer (Ptr<OutputStreamWrapper> stream,
                     uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Queue Disc size from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void CwndTracer (Ptr<OutputStreamWrapper> stream,
            uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Cwnd size from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void TraceCwnd (Ptr<OutputStreamWrapper> cwndStream)
{
  Config::ConnectWithoutContext ("/NodeList/5/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                 MakeBoundCallback (&CwndTracer, cwndStream));
}

static void TraceAggregator (std::size_t index, Ptr<const Packet> p, const Address& a)
{
  aggregatorBytes += p->GetSize ();
  if (aggregatorBytes >= ONE_MB - printLastXBytesReceived)
  {
    if (printLastXBytesReceived == 0)
    {
      finishTime = Simulator::Now ().GetMilliSeconds ();
      completionTimesStream << numFlows << ":" << finishTime  - firstFlowStart.GetMilliSeconds () << std::endl;
    } 
    else 
    {
      finishTime = Simulator::Now ().GetMilliSeconds ();
      completionTimesStream << aggregatorBytes << " : " << finishTime - firstFlowStart.GetMilliSeconds () << std::endl;
    }
  }
}



int main (int argc, char *argv[])
{
  std::string outputFilePath = "../outputs/";
  std::string tcpTypeId = "TcpDctcp";
  std::string outputFilename = "unnamed.txt";
  bool enableSwitchEcn = true;
  Time progressInterval = MicroSeconds (100);
  size_t numSenders = 9;
  CommandLine cmd (__FILE__);
  cmd.AddValue ("tcpTypeId", "ns-3 TCP TypeId", tcpTypeId);
  cmd.AddValue ("enableSwitchEcn", "enable ECN at switches", enableSwitchEcn);
  cmd.AddValue ("numFlows", "set the number of flows sent to aggregator", numFlows);
  cmd.AddValue ("outputFilePath", "set path for output files", outputFilePath);
  cmd.AddValue ("outputFilename", "set filename for output file", outputFilename);
  cmd.AddValue ("numSenders", "number of client host machines", numSenders);
  cmd.AddValue ("printLastXBytesReceived", 
                "print arrival times of bytes > (1MB - printLastXBytesReceived)", 
                printLastXBytesReceived);
  cmd.Parse (argc, argv);
  LogComponentEnable("DCTCP-PlusExperiment", LOG_LEVEL_DEBUG);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Time startTime = Seconds (0);
  Time stopTime = Seconds (5);

  AsciiTraceHelper asciiTraceHelper;
  std::string cwndStreamName = outputFilePath + "cwnd.tr";
  Ptr<OutputStreamWrapper> cwndStream = asciiTraceHelper.CreateFileStream (cwndStreamName);

  std::string qStreamName = outputFilePath + "q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  /******** Create Nodes ********/
  NS_LOG_DEBUG("Creating Nodes...");
  NS_LOG_DEBUG("using " << tcpTypeId << " with " << numFlows << " flows and " << 
                numSenders << " senders, outputs in " << outputFilePath);
  if (enableSwitchEcn) {
    NS_LOG_DEBUG("ecn switch enabled");
  }
  Ptr<Node> aggregator = CreateObject<Node> ();
  Ptr<Node> S1 = CreateObject<Node> ();

  size_t numIntermediateSwitches = 3;
  NodeContainer switches234;
  switches234.Create(numIntermediateSwitches);
  
  /* Need to decide whether we want numFlows servers or 9 servers*/
  NodeContainer senders;
  
  senders.Create(numSenders);
  
  /******** Create Channels and Net Devices ********/
  NS_LOG_DEBUG("Creating Channels and Net Devices...");
  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  link.SetChannelAttribute ("Delay", StringValue ("25us"));
  link.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  
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

  for (std::size_t i = 0; i < numSenders; i++)
    {
      Ptr<Node> sender = senders.Get (i);
      sendersToIntermediateSwitches.push_back (
        link.Install (sender, switches234.Get (i % numIntermediateSwitches))
      );
    }
  
  /******** Set TCP defaults ********/
  NS_LOG_DEBUG("Setting TCP/DCTCP Defaults...");

  uint32_t tcpSegmentSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
  // if (tcpTypeId != "TcpNewReno") {
  //   Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (2));
  //   GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  // }
  
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  
    Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  if (tcpTypeId == "TcpNewReno") {
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                        TypeIdValue (TypeId::LookupByName ("ns3::TcpClassicRecovery")));
  } else {
    // Set default parameters for RED queue disc
    Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (enableSwitchEcn));
    Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"));
    // ARED may be used but the queueing delays will increase; it is disabled
    // here because the SIGCOMM paper did not mention it
    // Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
    // Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
    Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
    Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (1500));
    // DCTCP+ paper used switches with 128KB of buffer for all tests
    // If every packet is 1500 bytes, ~85 packets can be stored in 128 KB
    // Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("340p")));
    Config::SetDefault ("ns3::RedQueueDisc::MaxSize", QueueSizeValue (QueueSize ("85p")));
    // DCTCP tracks instantaneous queue length only; so set QW = 1
    Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1));

    // Same as K for DCTCP+
    Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (22));
    Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (22)); // iterate through different values here
  }


  /******** Install Internet Stack ********/
  NS_LOG_DEBUG("Installing Internet Stack...");
  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tch;
  if (tcpTypeId == "TcpNewReno") {
    tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc",
                            //  "MaxSize", QueueSizeValue (QueueSize ("340p")));
                             "MaxSize", QueueSizeValue (QueueSize ("85p")));
  } else {

    // MinTh = 20, MaxTh = 60 recommended in ACM SIGCOMM 2010 DCTCP Paper
    // This yields a target queue depth of 250us at 1 Gb/s
    // DCTCP+ paper says 32KB K threshold = 21.84 ≈ 22 packets
    tch.SetRootQueueDisc ("ns3::RedQueueDisc",
                            "LinkBandwidth", StringValue ("1Gbps"),
                            "LinkDelay", StringValue ("25us"),
                            "MinTh", DoubleValue (22),
                            "MaxTh", DoubleValue (22));
  
  }
  QueueDiscContainer queueDiscs = tch.Install (S1ToA);
  queueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
                            MakeBoundCallback (&QueueOccupancyTracer, qStream));
  for (std::size_t i = 0; i < numIntermediateSwitches; i++)
    {
      tch.Install (intermediateSwitchesToS1[i]);
    }
  for (std::size_t i = 0; i < numSenders; i++)
    {
      tch.Install (sendersToIntermediateSwitches[i]);
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

  for (std::size_t i = 0; i < numSenders; i++)
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
  /*
    we'll use this random variable to randomize sender start times to simulate
    the aggregator's request reaching that sender after some small delay
  */
  Ptr<UniformRandomVariable> aggregatorRequestDelay = CreateObject<UniformRandomVariable> ();
  for (std::size_t i = 0; i < numFlows; i++) {
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", aggregatorAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));
    uint64_t maxBytes = i < ONE_MB % numFlows ? ONE_MB / numFlows + 1 : ONE_MB / numFlows;
    ftp.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    bulkSenders.push_back (ftp);
    ApplicationContainer senderApp = ftp.Install (senders.Get (i % numSenders));
    Time slightDelay = MicroSeconds(aggregatorRequestDelay->GetInteger(1, randomStartWindow));
    firstFlowStart = firstFlowStart < slightDelay ? firstFlowStart : slightDelay;
    senderApp.Start (startTime + slightDelay);
    senderApp.Stop (stopTime);
    senderApps.push_back (senderApp);
  }

  completionTimesStream.open (outputFilePath + outputFilename, std::ios::out | std::ios::app);
  aggSink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (&TraceAggregator, 0));
  
  /* Start tracing cwnd of the connection after the connection is established */
  Simulator::Schedule (traceStart, &TraceCwnd, cwndStream);
  
  NS_LOG_DEBUG("Starting simulation...");
  Simulator::Stop (stopTime);
  Simulator::Run ();

  completionTimesStream.close ();
  Simulator::Destroy ();
  NS_LOG_DEBUG("Finished in: " << finishTime << "ms");
  return 0;
}
