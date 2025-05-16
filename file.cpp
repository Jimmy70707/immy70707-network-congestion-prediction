#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

int main() {
  // Create nodes
  NodeContainer clients;
  clients.Create(4);
  
  NodeContainer servers;
  servers.Create(2);
  
  NodeContainer hub;
  hub.Create(1);  // Central router

  // Create channels
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  PointToPointHelper slowP2P;
  slowP2P.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  slowP2P.SetChannelAttribute("Delay", StringValue("10ms"));

  NetDeviceContainer clientDevices[4];
  NetDeviceContainer serverDevices[2];
  NetDeviceContainer hubDevices[6];

  // Connect clients to hub
  for (int i = 0; i < 4; ++i) {
    NodeContainer pair = NodeContainer(clients.Get(i), hub.Get(0));
    NetDeviceContainer devices = p2p.Install(pair);
    
    clientDevices[i] = devices.Get(0);
    hubDevices[i] = devices.Get(1);
  }

  // Connect servers to hub
  for (int i = 0; i < 2; ++i) {
    NodeContainer pair = NodeContainer(servers.Get(i), hub.Get(0));
    NetDeviceContainer devices = slowP2P.Install(pair);
    
    serverDevices[i] = devices.Get(0);
    hubDevices[4+i] = devices.Get(1);
  }

  // Install internet stack
  InternetStackHelper stack;
  stack.InstallAll();

  // Assign IP addresses
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces[6];

  for (int i = 0; i < 6; ++i) {
    std::ostringstream subnet;
    subnet << "10.1." << i+1 << ".0";
    address.SetBase(subnet.str().c_str(), "255.255.255.0");
    interfaces[i] = address.Assign(hubDevices[i]);
  }

  // Set up global routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Install applications
  // UDP Echo Server on server 0
  UdpEchoServerHelper echoServer(9);
  ApplicationContainer serverApp = echoServer.Install(servers.Get(0));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  // BulkSendApplication on server 1
  BulkSendHelper bulkSender("ns3::TcpSocketFactory", 
                            InetSocketAddress(interfaces[5].GetAddress(0), 80));
  bulkSender.SetAttribute("MaxBytes", UintegerValue(10485760)); // 10MB
  ApplicationContainer bulkApp = bulkSender.Install(servers.Get(1));
  bulkApp.Start(Seconds(2.0));
  bulkApp.Stop(Seconds(10.0));

  // UDP Clients
  for (int i = 0; i < 4; ++i) {
    UdpEchoClientHelper echoClient(interfaces[4].GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    
    ApplicationContainer clientApp = echoClient.Install(clients.Get(i));
    clientApp.Start(Seconds(2.0 + i*0.5));
    clientApp.Stop(Seconds(10.0));
  }

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
