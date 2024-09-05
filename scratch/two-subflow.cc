#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mp-tcp-packet-sink.h"
#include "ns3/mp-tcp-bulk-send-application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MpTcpExample");


void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newCwnd << std::endl;
}

void
RttChange(Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newRtt.GetMilliSeconds() << std::endl;
}

int main(int argc, char *argv[])
{
  LogComponentEnable("MpTcpExample", LOG_LEVEL_INFO);
  LogComponentEnable("MpTcpSocketBase", LOG_LEVEL_INFO);

  Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
  //Config::SetDefault("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(100));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(MpTcpSocketBase::GetTypeId()));
  Config::SetDefault("ns3::MpTcpSocketBase::MaxSubflows", UintegerValue(2));

  NodeContainer nodes;
  nodes.Create(6);  // sender, receiver, 4 routers

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));

  NetDeviceContainer devices01 = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
  NetDeviceContainer devices02 = pointToPoint.Install(nodes.Get(0), nodes.Get(2));
  NetDeviceContainer devices13 = pointToPoint.Install(nodes.Get(1), nodes.Get(3));
  NetDeviceContainer devices24 = pointToPoint.Install(nodes.Get(2), nodes.Get(4));
  NetDeviceContainer devices35 = pointToPoint.Install(nodes.Get(3), nodes.Get(5));
  NetDeviceContainer devices45 = pointToPoint.Install(nodes.Get(4), nodes.Get(5));

  InternetStackHelper stack;
  stack.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces02 = address.Assign(devices02);
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces13 = address.Assign(devices13);
  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces24 = address.Assign(devices24);
  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces35 = address.Assign(devices35);
  address.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces45 = address.Assign(devices45);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  uint16_t port = 9;
  Address sinkAddress(InetSocketAddress(interfaces35.GetAddress(1), port));
  MpTcpPacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(5));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(1.0));

  MpTcpBulkSendHelper source("ns3::TcpSocketFactory", sinkAddress);
  source.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApps = source.Install(nodes.Get(0));
  sourceApps.Start(Seconds(0.0));
  sourceApps.Stop(Seconds(1.0));


  Simulator::Stop(Seconds(1.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
