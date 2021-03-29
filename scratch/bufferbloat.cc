/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Stanford University
 *
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
 * Author: Serhat Arslan <sarslan@stanford.edu>
 *
 * Bufferbloat Topology
 *
 *    h1-----------------s0-----------------h2
 *
 *  Usage (e.g.):
 *    ./waf --run 'bufferbloat --time=60 --bwNet=10 --delay=1 --maxQ=100'
 */

// TODO: Don't just read the TODO sections in this code.  Remember that one of
//       the goals of this assignment is for you to learn how to use NS-3 :-)

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BufferBloatExample");

static double TRACE_START_TIME = 0.05;

/* Tracing is one of the most valuable features of a simulation environment.
 * It means we can get to see evolution of any value / state we are interested
 * throughout the simulation. Basically, in NS-3, you set some tracing options
 * for pre-defined TraceSources and provide a function that defines what to do
 * when the traced value changes.
 * The helper functions below are used to trace particular values throughout
 * the simulation. Make sure to take a look at how they are used in the main
 * function. You can learn more about tracing at
 * https://www.nsnam.org/docs/tutorial/html/tracing.html If you are going to
 * work with NS-3 in the future, you will definitely need to read this page.
 */

static void
QueueOccupancyTracer (Ptr<OutputStreamWrapper> stream,
                     uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Queue Disc size from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void
CwndTracer (Ptr<OutputStreamWrapper> stream,
            uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Cwnd size from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void
TraceCwnd (Ptr<OutputStreamWrapper> cwndStream)
{
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                 MakeBoundCallback (&CwndTracer, cwndStream));
}

static void
RttTracer (Ptr<OutputStreamWrapper> stream,
           Time oldval, Time newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Rtt from " << oldval.GetMilliSeconds () <<
               " to " << newval.GetMilliSeconds ());

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval.GetMilliSeconds () << std::endl;
}

static void
TraceRtt (Ptr<OutputStreamWrapper> rttStream)
{
  // TODO: In the TraceCwnd function above, you learned how to trace congestion
  //       window size of a TCP socket. Take a look at the documentation for
  //       TCP Sockets and find the name of the TraceSource in order to trace
  //       the round trip time (delay) experienced by the flow.
  //
  // HINT: TCP Socket is implemented on multiple classes in NS3. The trace
  //       source you are looking for might be in any of them.
  /* Note how the path is constructed for configuring the TraceSource. NS-3
   * keeps a hierarchical list of all available modules created for the
   *simulation
   */
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/__FILL_IN_HERE__",
                                 MakeBoundCallback (&RttTracer, rttStream));
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;

  /* Start by setting default variables. Feel free to play with the input
   * arguments below to see what happens.
   */
  int bwHost = 1000; // Mbps
  int bwNet = 10; // Mbps
  int delay = 1; // milliseconds
  int time = 60; // seconds
  int maxQ = 100; // packets
  std::string transport_prot = "TcpNewReno";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("bwHost", "Bandwidth of host links (Mb/s)", bwHost);
  cmd.AddValue ("bwNet", "Bandwidth of bottleneck (network) link (Mb/s)", bwNet);
  cmd.AddValue ("delay", "Link propagation delay (ms)", delay);
  cmd.AddValue ("time", "Duration (sec) to run the experiment", time);
  cmd.AddValue ("maxQ", "Max buffer size of network interface in packets", maxQ);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
                "TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, "
                "TcpScalable, TcpVeno, TcpBic, TcpYeah, TcpIllinois, "
                "TcpWestwood, TcpLedbat, TcpLp, TcpDctcp",
                transport_prot);
  cmd.Parse (argc, argv);

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("BufferBloatExample", LOG_LEVEL_DEBUG);

  std::string bwHostStr = std::to_string(bwHost) + "Mbps";
  std::string bwNetStr = std::to_string(bwNet) + "Mbps";
  std::string delayStr = std::to_string(delay) + "ms";
  std::string maxQStr = std::to_string(maxQ) + "p";
  transport_prot = std::string ("ns3::") + transport_prot;

  NS_LOG_DEBUG("Bufferbloat Simulation for:" <<
               " bwHost=" << bwHostStr << " bwNet=" << bwNetStr <<
               " delay=" << delayStr << " time=" << time << "sec" <<
               " maxQ=" << maxQStr << " protocol=" << transport_prot);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  std::string dir = "outputs/bb-q" + std::to_string(maxQ) + "/";

  std::string qStreamName = dir + "q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  std::string cwndStreamName = dir + "cwnd.tr";
  Ptr<OutputStreamWrapper> cwndStream;
  cwndStream = asciiTraceHelper.CreateFileStream (cwndStreamName);

  std::string rttStreamName = dir + "rtt.tr";
  Ptr<OutputStreamWrapper> rttStream;
  rttStream = asciiTraceHelper.CreateFileStream (rttStreamName);

  /* In order to run simulations in NS-3, you need to set up your network all
   * the way from the physical layer to the application layer. But don't worry!
   * NS-3 has very nice classes to help you out at every layer of the network.
   */

  /******** Create Nodes ********/
  /* Nodes are the used for end-hosts, switches etc. */
  NS_LOG_DEBUG("Creating Nodes...");

  NodeContainer nodes;
  // TODO: Read documentation for NodeContainer object and create 3 nodes.
  nodes.__FILL_IN_HERE__;

  Ptr<Node> h1 = nodes.Get(0);
  Ptr<Node> s0 = nodes.Get(1);
  Ptr<Node> h2 = nodes.Get(2);

  /******** Create Channels ********/
  /* Channels are used to connect different nodes in the network. There are
   * different types of channels one can use to simulate different environments
   * such as ethernet or wireless. In our case, we would like to simulate a
   * cable that directly connects two nodes. These cables have particular
   * properties (ie. propagation delay and link rate) and they need to be set
   * before installing them on nodes.
   */
  NS_LOG_DEBUG("Configuring Channels...");

  PointToPointHelper hostLink;
  hostLink.SetDeviceAttribute ("DataRate", StringValue (bwHostStr));
  hostLink.SetChannelAttribute ("Delay", StringValue (delayStr));
  hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bwNetStr));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (delayStr));
  bottleneckLink.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue ("1p"));

  /******** Create NetDevices ********/
  NS_LOG_DEBUG("Creating NetDevices...");

  /* When channels are installed in between nodes, NS-3 creates NetDevices for
   * the associated nodes that simulate the Network Interface Cards connecting
   * the channel and the node.
   */
  // TODO: Read documentation for PointToPointHelper object and install the
  //       links created above in between correct nodes.
  NetDeviceContainer h1s0_NetDevices = hostLink.__FILL_IN_HERE__;
  NetDeviceContainer s0h2_NetDevices = bottleneckLink.__FILL_IN_HERE__;

  /******** Set TCP defaults ********/
  PppHeader ppph;
  Ipv4Header ipv4h;
  TcpHeader tcph;
  uint32_t tcpSegmentSize = h1s0_NetDevices.Get (0)->GetMtu ()
                            - ppph.GetSerializedSize ()
                            - ipv4h.GetSerializedSize ()
                            - tcph.GetSerializedSize ();

  Config::SetDefault ("ns3::TcpSocket::SegmentSize",
                      UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName ("ns3::TcpClassicRecovery")));

  // Select TCP variant
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid),
                       "TypeId " << transport_prot << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                      TypeIdValue (TypeId::LookupByName (transport_prot)));

  /******** Install Internet Stack ********/
  NS_LOG_DEBUG("Installing Internet Stack...");

  InternetStackHelper stack;
  stack.InstallAll ();

  // TODO: Read documentation for PfifoFastQueueDisc and use the correct
  //       attribute name to set the size of the bottleneck queue.
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc",
                             "__FILL_IN_HERE__", StringValue(maxQStr));

  tchPfifo.Install (h1s0_NetDevices);
  QueueDiscContainer s0h2_QueueDiscs = tchPfifo.Install (s0h2_NetDevices);
  /* Trace Bottleneck Queue Occupancy */
  s0h2_QueueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
                            MakeBoundCallback (&QueueOccupancyTracer, qStream));

  /* Set IP addresses of the nodes in the network */
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer h1s0_interfaces = address.Assign (h1s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer s0h2_interfaces = address.Assign (s0h2_NetDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /******** Setting up the Application ********/
  NS_LOG_DEBUG("Setting up the Application...");

  /* The receiver application */
  uint16_t receiverPort = 5001;
  // TODO: Provide the correct IP address below for the receiver
  AddressValue receiverAddress (InetSocketAddress (__FILL_IN_HERE__,
                                                   receiverPort));
  PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                   receiverAddress.Get());
  receiverHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));

  // TODO: Install the receiver application on the correct host.
  ApplicationContainer receiverApp = receiverHelper.Install (__FILL_IN_HERE__);
  receiverApp.Start (Seconds (0.0));
  receiverApp.Stop (Seconds ((double)time));

  /* The sender Application */
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  // TODO: Read documentation for BulkSendHelper to figure out the name of the
  //       Attribute for setting the destination address for the sender.
  ftp.SetAttribute ("__FILL_IN_HERE__", receiverAddress);
  ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));

  // TODO: Install the source application on the correct host.
  ApplicationContainer sourceApp = ftp.Install (__FILL_IN_HERE__);
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds ((double)time));

  /* Start tracing cwnd of the connection after the connection is established */
  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceCwnd, cwndStream);

  /* Start tracing the RTT after the connection is established */
  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceRtt, rttStream);

  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  // TODO: Up until now, you have only set up the simulation environment and
  //       described what kind of events you want NS3 to simulate. However
  //       you have actually not run the simulation yet. Complete the command
  //       below to run it.
  Simulator::__FILL_IN_HERE__;
  Simulator::Destroy ();
  return 0;
}
