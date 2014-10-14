//
//
//  Mobile IPv6 (ns-3-dce-umip),  (by Hajime Tazaki, Mathieu Lackage, ..)
//
//
//  Simulation Topology:
//  Scenario: CN pings (or sends traffic) to MN
//
//                                        1        2
//                                 +------+        +----+-----------+
//                                 |  CN  | - - - -|       IR       | 
//                                 +------+   l1   +-------+--------+
//                                             	   |3     5|      7|
//		                          - - - - -+       |       +- - - - - - -+
//			                 |   l2         l3 |             l4      |
//	       	                       4 |                6|                   8 |
//    +----+ b    c +-----+ a     9 +----+---+       +-----+----+          +-----+----+
//    | HA |- - - - | AP1 |- - - - -|   AR1  |       | AR2 (AP2)|          | AR3 (AP3)|
//    +----+   l6   +-----+    l5   +---+----+       +-----+----+          +-----+----+
//                    d\                                  / e                   / f
//                      --  wifi                         --                    --
//     	  Home	          \                              /                     /
//     	  network	    10                       Foreign               Foreign
//        2001:5::/64    +----+    moving            network 2             network 3
//                       | MN |  ========>                  
//                       +----+                     
//
//  HoA: 2001:5::200:ff:fe00:202
//  HA:  2001:5::200:ff:fe00:b
//   
//  Goran Shekerov (g_sekerov@yahoo.com)
//


#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/wifi-module.h>
#include "ns3/csma-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/dce-module.h"
#include "ns3/mip6d-helper.h"
#include "ns3/quagga-helper.h"
#include "ns3/bridge-helper.h"
#include <ns3/tpa-module.h>
#include <iostream>
#include <ns3/random-variable-stream.h>
#include <ns3/config-store-module.h>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("ipv6 test");

static void RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

static void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address)
{
  std::ostringstream oss;
  oss << "-f inet6 addr add " << address << " dev " << name;
  RunIp (node, at, oss.str ());
}

// some global variables :-)

  Tpa stats; // Declare global Tpa object named stats witch is 
             //used to load informations for analyzing performances

  double startAppTime;
  double stopAppTime;
  double endSimulationTime;

  // for the background nodes    
  NodeContainer bNodes2;
  NodeContainer bNodes3;
  NetDeviceContainer wifi_bck_devices2;
  NetDeviceContainer wifi_bck_devices3;

//****Callbacks
void TxCallback(Ptr<const Packet> tporig)
{ 
  double timeNow=Simulator::Now().GetMilliSeconds();
  if ( (timeNow/1000.0) < (stopAppTime - 0.1))
    {
      stats.LoadSentPacket(tporig, timeNow);
    }
}

void RxCallback(Ptr<const Packet> rporig)
{
  double timeNow=Simulator::Now().GetMilliSeconds();
  if ( (timeNow/1000.0) < (stopAppTime - 0.1))
    {
			stats.LoadReceivedPacket(rporig, timeNow);
    }
}

void RxControlCallback(Ptr<const Packet> rporig)
{
  double timeNow=Simulator::Now().GetMilliSeconds();
  stats.LoadControlPacket(rporig, timeNow);
}

void XPositionCallback(Ptr<const MobilityModel> mob_model)
{
  Vector position=mob_model->GetPosition();
  double timeNow=Simulator::Now().GetMilliSeconds();
  std::cout << "MN is at position x=" << position.x << "   time=" << timeNow/1000 << "s" << std::endl;
}



int main(int argc, char* argv[])
{

// ****Declaring parametars and parsing

  int      mn_movement = 2;    // 1: MN is static  2: MN is mobile and moves between networks
  int      mn_path = 1;        // for mobile MN -  1: HN -> FN2  2:FN2 -> FN3  3:FN2 -> HN
  int      mn_location = 1;    // for static MN -  1:HN   2:FN2   3:FN3 
           
  int      V=5;               // Velocity in km/h
  int      traffic_type = 1;   // 1:PING  2:UDPCBR  3:TCPCBR   4:VOIP  5:LIVE_VIDEO  6.VIDEO

  int      ra_interval = 1;    // Routing Advertisement interval in seconds
  bool     ro_enable = true;             // enable mipv6 in CN; Routing Optimization

  bool     background_nodes = false;  // if enabled and want pcap or netanim, manually uncoment appropriate blocks 
  int      bN = 1; // number of background nodes in each network (FN2, FN3); don't know why but fails for bN=5 :-)

  bool     callbacks_enable = true;
  bool     output_label_enable = true;
  bool     print_throughput = false;
  bool     pcap_enable = true;
  bool     anim_enable = false;


  CommandLine cmd;
  cmd.AddValue ("mn_movement", "1 mn is static or 2 mn moves between networks", mn_movement);
  cmd.AddValue ("mn_path", "for mobile MN -  1: HN -> FN2  2:FN2 -> FN3  3:FN2 -> HN", mn_path);
  cmd.AddValue ("mn_location", "for static MN -  1:HN   2:FN2   3:FN3", mn_location);
  cmd.AddValue ("V", "Velocity in km/h", V);
  cmd.AddValue ("traffic_type", "1:PING  2:UDPCBR  3:TCPCBR   4:VOIP  5:LIVE_VIDEO  6.VIDEO", traffic_type);
  cmd.AddValue ("ra_interval", "Routing Advertisement interval in seconds", ra_interval);
  cmd.AddValue ("ro_enable", "enable mipv6 in CN; Routing Optimization", ro_enable);
  cmd.AddValue ("background_nodes", "bool enable/disable background nodes", background_nodes);
  cmd.AddValue ("bN", "Number of background nodes in FN2 and FN3 each", bN);
  cmd.AddValue ("callbacks_enable", "callbacks_enable", callbacks_enable);
  cmd.AddValue ("output_label_enable", "output_label_enable", output_label_enable);
  cmd.AddValue ("print_throughput", "print_throughput", print_throughput);
  cmd.AddValue ("pcap_enable", "pcap_enable", pcap_enable);
  cmd.AddValue ("anim_enable", "anim_enable", anim_enable);
  cmd.Parse (argc,argv);

  //Set the traffic type PING, UDPCBR, VOIP or VIDEO_STREAM
  std::string trafficType;
  switch (traffic_type)
    {
      case 1: trafficType = "PING"; break;
      case 2: trafficType = "UDPCBR"; break;
      case 3: trafficType = "TCPCBR"; break;
      case 4: trafficType = "VOIP"; break;
      case 5: trafficType = "VIDEO_S"; break;      
    }
  stats.SetTrafficType (trafficType);
  if (!output_label_enable){ stats.m_enable_column_labels = false;}

// Some checks dce 1.2 doesn't support TCP UDP applications yet
  if (traffic_type != 1 and ro_enable == true) { std::cout << "RO not implemented in Tpa; disable RO for this traffic_type!" << std::endl; return 0;}


// ****Create Nodes

  Ptr<Node> cn  = CreateObject<Node> (); //Correspondence Node
  Ptr<Node> ir  = CreateObject<Node> (); //Internet Router
  Ptr<Node> ha  = CreateObject<Node> (); //Home Agent
  Ptr<Node> ap1 = CreateObject<Node> (); //Access Point 1
  Ptr<Node> ar1 = CreateObject<Node> (); //Access Router 1
  Ptr<Node> ar2 = CreateObject<Node> (); //Access Router 2, the AP2 is in this node
  Ptr<Node> ar3 = CreateObject<Node> (); //Access Router 3, the AP3 is in this node
  Ptr<Node> mn  = CreateObject<Node> (); //Mobile Node
  NodeContainer cn_nodes(cn); 
  NodeContainer ir_nodes(ir);
  NodeContainer ha_nodes(ha); 
  NodeContainer ap_nodes(ap1, ar2, ar3);
  NodeContainer ar_nodes(ar1, ar2, ar3); 
  NodeContainer mn_nodes(mn);
  NodeContainer ap1_node(ap1);
  NodeContainer all_nodes(cn_nodes, ir_nodes, ha_nodes, ar_nodes, mn_nodes); // without AP1
  NodeContainer all_linux_nodes(ir_nodes, ha_nodes, ar_nodes, mn_nodes); // without AP1

// ****Mobility 

  double ApDistance=200.0;
  double staVelocity=0.2777778 * V; // 1km/h=0.277778m/s
  Ptr<UniformRandomVariable> xVar = CreateObject<UniformRandomVariable> ();
  double initializationTime = xVar -> GetValue(15, 15);
  //double initializationTime=15.0;
  double walkingTime = (25 + ApDistance + 25) / staVelocity;
  if (V == 0) {walkingTime = 60;}
  // std::cout << initializationTime << std::endl;
  // std::cout << "walking time=" << walkingTime << std::endl;
  // std::cout << "V =" << V << std::endl;
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
                           //    x    y    z
  positionAlloc->Add (Vector (130.0, 150.0, 0.0));  // CN
  positionAlloc->Add (Vector (300.0, 150.0, 0.0));  // IR
  positionAlloc->Add (Vector ( 90.0, 100.0, 0.0));  // HA
  positionAlloc->Add (Vector (100.0, 100.0, 0.0));  // AP1
  positionAlloc->Add (Vector (130.0, 100.0, 0.0));  // AR1
  positionAlloc->Add (Vector (100.0 + ApDistance, 100.0, 0.0));  // AP2
  positionAlloc->Add (Vector (100.0 + ApDistance + ApDistance, 100.0, 0.0));  // AP2
  if (mn_movement == 1)  // 1: static
    { switch (mn_location)  // for static MN locations
        { case 1: positionAlloc->Add (Vector (100.0, 80.0, 0.0)); break;  //  MN in home 
          case 2: positionAlloc->Add (Vector (100.0 + ApDistance, 80.0, 0.0)); break;  //  MN in foreign 2
          case 3: positionAlloc->Add (Vector (100.0 + ApDistance + ApDistance, 80.0, 0.0)); break;  //  MN in foreign 3
        }
    }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (cn_nodes);
  mobility.Install (ir_nodes);
  mobility.Install (ha_nodes);
  mobility.Install (ap1_node);
  mobility.Install (ar_nodes);
  if (mn_movement == 1) {mobility.Install (mn);} 
  if (mn_movement == 2) // MN moves between networks
    {
      mobility.SetMobilityModel ("ns3::WaypointMobilityModel");
      mobility.Install (mn);    
 
      Waypoint wp1;
      Waypoint wp2;
      Waypoint wp3;

      switch (mn_path)
        {
          case 1: // HN -> FN2
            wp1.time = Seconds (0.0);                              wp1.position = Vector (75.0, 50.0, 0.0);
            wp2.time = Seconds (initializationTime);               wp2.position = Vector (75.0, 50.0, 0.0);
            wp3.time = Seconds (initializationTime + walkingTime); wp3.position = Vector (125.0 + ApDistance, 50.0, 0.0);
            break;   
          case 2: // FN2 -> FN3
            wp1.time = Seconds (0.0);                              wp1.position = Vector (75.0 + ApDistance, 50.0, 0.0);
            wp2.time = Seconds (initializationTime);               wp2.position = Vector (75.0 + ApDistance, 50.0, 0.0);
            wp3.time = Seconds (initializationTime + walkingTime); wp3.position = Vector (125.0 + ApDistance + ApDistance, 50.0, 0.0);
            break; 
            case 3: // FN2 -> HN
            wp1.time = Seconds (0.0);                              wp1.position = Vector (125.0 + ApDistance, 50.0, 0.0);
            wp2.time = Seconds (initializationTime);               wp2.position = Vector (125.0 + ApDistance, 50.0, 0.0);
            wp3.time = Seconds (initializationTime + walkingTime); wp3.position = Vector (75.0, 50.0, 0.0);        
            break;
        }

      Ptr<WaypointMobilityModel> wpmob = mn -> GetObject<WaypointMobilityModel> ();
      wpmob->AddWaypoint (wp1);
      wpmob->AddWaypoint (wp2);
      wpmob->AddWaypoint (wp3);
    } 


// ****Create and install devices
  // csma
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue("100Mbps"));
  if (trafficType != "UDPCBR") {csma.SetChannelAttribute ("Delay", StringValue ("10ms"));} 
  // for UDPCBR, if delay > 0.6ms the packet loss is huge?
  NetDeviceContainer l1_devices = csma.Install (NodeContainer( cn, ir));   //link 1
  NetDeviceContainer l2_devices = csma.Install (NodeContainer( ir, ar1));  //l2
  NetDeviceContainer l3_devices = csma.Install (NodeContainer( ir, ar2));  //l3
  NetDeviceContainer l4_devices = csma.Install (NodeContainer( ir, ar3));  //l4
  
  csma.SetChannelAttribute ("Delay", StringValue ("10us"));
  NetDeviceContainer l5_devices = csma.Install (NodeContainer(ar1, ap1));  // ar1-ap1 devices
  NetDeviceContainer l6_devices = csma.Install (NodeContainer( ha, ap1));  // ha-ap1 devices

  // wifi
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  //Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::ApWifiMac");//,
                //"Ssid", SsidValue (ssid));
  NetDeviceContainer  wifi_ap_devices = wifi.Install (phy, mac, ap_nodes);

  mac.SetType ("ns3::StaWifiMac",
               //"Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
   NetDeviceContainer  wifi_sta_devices = wifi.Install (phy, mac, mn);

  //AP1 switch (bridge), L2 device 

  NetDeviceContainer switchDevices;
  switchDevices.Add (l5_devices.Get (1));
  switchDevices.Add (l6_devices.Get (1));
  switchDevices.Add (wifi_ap_devices.Get (0));
  BridgeHelper sw_bridge;
  sw_bridge.Install (ap1, switchDevices);




// ****Internet stack - DCE linux stack
  DceManagerHelper dceManager;  
  dceManager.SetTaskManagerAttribute ("FiberManagerType", EnumValue (0));
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  if (trafficType == "PING") {dceManager.Install (all_nodes);}
  if (trafficType != "PING") {dceManager.Install (all_linux_nodes);}
  

// **IPv6 configuration

  // CN
  if (trafficType == "PING")
   {
     AddAddress (cn, Seconds (0.1), "sim0", "2001:1::200:ff:fe00:1/64");
     RunIp (cn, Seconds (0.11), "link set lo up");
     RunIp (cn, Seconds (0.11), "link set sim0 up");
     RunIp (cn, Seconds (0.15), "-6 route add default via 2001:1::200:ff:fe00:2 dev sim0");
   }

  if (trafficType != "PING")
    {
      InternetStackHelper internet_helper;
      internet_helper.SetIpv4StackInstall (false);  //Ping6 does not work without this line :)
      internet_helper.Install (cn);
     
      Ipv6AddressHelper ipv6_add_helper;
      ipv6_add_helper.SetBase (Ipv6Address ("2001:1::"), 64, Ipv6Address("::1"));
      Ipv6InterfaceContainer l1_interfaces = ipv6_add_helper.Assign (l1_devices.Get (0));

      Ipv6StaticRoutingHelper routingHelper_cn;
      Ptr<Ipv6StaticRouting> cn_routing  = routingHelper_cn.GetStaticRouting (cn_nodes.Get (0)->GetObject<Ipv6> ());
      cn_routing->SetDefaultRoute(Ipv6Address ("2001:1::200:ff:fe00:2"), 1, Ipv6Address ("::"), 0);
    }

  // IR
  Ptr<LinuxSocketFdFactory> kern = ir->GetObject<LinuxSocketFdFactory>();
  Simulator::ScheduleWithContext (ir->GetId (), Seconds (0.1),
                                  MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                             ".net.ipv6.conf.all.forwarding", "1"));
  AddAddress (ir, Seconds (2.1), "sim0", "2001:1::200:ff:fe00:2/64");
  RunIp (ir, Seconds (2.11), "link set lo up");
  RunIp (ir, Seconds (2.11), "link set sim0 up");
  
  AddAddress (ir, Seconds (2.12), "sim1", "2001:2::200:ff:fe00:3/64");
  RunIp (ir, Seconds (2.13), "link set sim1 up");

  AddAddress (ir, Seconds (2.14), "sim2", "2001:3::200:ff:fe00:5/64");
  RunIp (ir, Seconds (2.15), "link set sim2 up");

  AddAddress (ir, Seconds (2.16), "sim3", "2001:4::200:ff:fe00:7/64");
  RunIp (ir, Seconds (2.17), "link set sim3 up");
  

  RunIp (ir, Seconds (2.18), "-6 route add 2001:5::/64 via 2001:2::200:ff:fe00:4 dev sim1");
  RunIp (ir, Seconds (2.19), "-6 route add 2001:6::/64 via 2001:3::200:ff:fe00:6 dev sim2");
  RunIp (ir, Seconds (2.20), "-6 route add 2001:7::/64 via 2001:4::200:ff:fe00:8 dev sim3");
  
  // HA
  AddAddress (ha, Seconds (0.1), "sim0", "2001:5::200:ff:fe00:b/64");
  RunIp (ha, Seconds (0.11), "link set lo up");
  RunIp (ha, Seconds (0.12), "link set sim0 up");
  RunIp (ha, Seconds (3.0), "link set ip6tnl0 up");
  RunIp (ha, Seconds (3.2), "-6 route add default via 2001:5::200:ff:fe00:9 dev sim0");

  // AR1
  kern = ar1->GetObject<LinuxSocketFdFactory>();
  Simulator::ScheduleWithContext (ar1->GetId (), Seconds (0.1),
                                 MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                             ".net.ipv6.conf.all.forwarding", "1"));
  AddAddress (ar1, Seconds (0.1), "sim0", "2001:2::200:ff:fe00:4/64");
  RunIp (ar1, Seconds (0.11), "link set lo up");
  RunIp (ar1, Seconds (0.11), "link set sim0 up");

  AddAddress (ar1, Seconds (0.12), "sim1", "2001:5::200:ff:fe00:9/64");
  RunIp (ar1, Seconds (0.13), "link set sim1 up");

  RunIp (ar1, Seconds (0.15), "-6 route add 2001:1::/64 via 2001:2::200:ff:fe00:3 dev sim0");
  RunIp (ar1, Seconds (0.16), "-6 route add 2001:3::/64 via 2001:2::200:ff:fe00:3 dev sim0");
  RunIp (ar1, Seconds (0.17), "-6 route add 2001:4::/64 via 2001:2::200:ff:fe00:3 dev sim0");
  RunIp (ar1, Seconds (0.17), "-6 route add 2001:6::/64 via 2001:2::200:ff:fe00:3 dev sim0");
  RunIp (ar1, Seconds (0.17), "-6 route add 2001:7::/64 via 2001:2::200:ff:fe00:3 dev sim0");

  // AR2
  kern = ar2->GetObject<LinuxSocketFdFactory>();
  Simulator::ScheduleWithContext (ar2->GetId (), Seconds (0.1),
                                  MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                             ".net.ipv6.conf.all.forwarding", "1"));
  AddAddress (ar2, Seconds (0.1), "sim0", "2001:3::200:ff:fe00:6/64");
  RunIp (ar2, Seconds (0.11), "link set lo up");
  RunIp (ar2, Seconds (0.11), "link set sim0 up");

  AddAddress (ar2, Seconds (0.12), "sim1", "2001:6::200:ff:fe00:e/64");
  RunIp (ar2, Seconds (0.13), "link set sim1 up");

  RunIp (ar2, Seconds (0.14), "-6 route add 2001:1::/64 via 2001:3::200:ff:fe00:5 dev sim0");
  RunIp (ar2, Seconds (0.14), "-6 route add 2001:2::/64 via 2001:3::200:ff:fe00:5 dev sim0");
  RunIp (ar2, Seconds (0.14), "-6 route add 2001:4::/64 via 2001:3::200:ff:fe00:5 dev sim0");
  RunIp (ar2, Seconds (0.14), "-6 route add 2001:5::/64 via 2001:3::200:ff:fe00:5 dev sim0");
  RunIp (ar2, Seconds (0.14), "-6 route add 2001:7::/64 via 2001:3::200:ff:fe00:5 dev sim0");

  // AR3
  kern = ar3->GetObject<LinuxSocketFdFactory>();
  Simulator::ScheduleWithContext (ar3->GetId (), Seconds (0.1),
                                  MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                             ".net.ipv6.conf.all.forwarding", "1"));
  AddAddress (ar3, Seconds (0.1), "sim0", "2001:4::200:ff:fe00:8/64");
  RunIp (ar3, Seconds (0.11), "link set lo up");
  RunIp (ar3, Seconds (0.11), "link set sim0 up");

  AddAddress (ar3, Seconds (0.12), "sim1", "2001:7::200:ff:fe00:f/64");
  RunIp (ar3, Seconds (0.13), "link set sim1 up");

  RunIp (ar3, Seconds (0.14), "-6 route add 2001:1::/64 via 2001:4::200:ff:fe00:7 dev sim0");
  RunIp (ar3, Seconds (0.14), "-6 route add 2001:2::/64 via 2001:4::200:ff:fe00:7 dev sim0");
  RunIp (ar3, Seconds (0.14), "-6 route add 2001:3::/64 via 2001:4::200:ff:fe00:7 dev sim0");
  RunIp (ar3, Seconds (0.14), "-6 route add 2001:5::/64 via 2001:4::200:ff:fe00:7 dev sim0");
  RunIp (ar3, Seconds (0.14), "-6 route add 2001:6::/64 via 2001:4::200:ff:fe00:7 dev sim0");

  // MN
  // IPv6 address autoconfigured from RAs
  //AddAddress (mn, Seconds (0.1), "sim0", "fe80::200:ff:fe00:11/64");
  RunIp (mn, Seconds (5.11), "link set lo up");
  RunIp (mn, Seconds (5.11), "link set sim0 up");
  RunIp (mn, Seconds (7.12), "link set ip6tnl0 up");
  // default gateway autoconfigured from RAs
  //RunIp (mn, Seconds (3.15), "-6 route add default via 2001:5::200:ff:fe00:9 dev sim0");


  // Print interfaces information 
  //RunIp (ir, Seconds (20), "route show table all"); //this output can be find in ..ns-3-dce/files-0/var/log/xxxx
  //RunIp (mn, Seconds (20), "address");



  //**** umip daemon
  
  Mip6dHelper mip6d;

  // HA
  mip6d.EnableHA(ha_nodes);
  mip6d.Install(ha_nodes);

  // MN
  mip6d.AddHomeAgentAddress (mn, Ipv6Address("2001:5::200:ff:fe00:b"));
  mip6d.AddHomeAddress (mn, Ipv6Address("2001:5::200:ff:fe00:202"), Ipv6Prefix(64));
  mip6d.AddEgressInterface (mn, "sim0");
  mip6d.Install(mn_nodes);

  // CN-MIPv6
  if (ro_enable)
    {  
      Mip6dHelper mip6d_cn;
      mip6d_cn.cn_mipv6 = true;
      mip6d_cn.EnableHA(ir_nodes);
      mip6d_cn.Install(ir_nodes);
      mip6d_cn.AddHomeAgentAddress (cn, Ipv6Address("2001:1::200:ff:fe00:2"));
      mip6d_cn.AddHomeAddress (cn, Ipv6Address("2001:1::200:ff:fe00:202"), Ipv6Prefix(64));
      mip6d_cn.AddEgressInterface (cn, "sim0");
      mip6d_cn.Install(cn_nodes);
    }



  //**** Routing Advertisement with quagga daemon

  /**
   * The RA interval can be changed in quagga-helper.cc line 935, or via manual configuration,
   * it is multiple of 1 sec.
   *
   * Don't mess with these quagga lines :) might end with BA error 132 - not home subnet, 
   * its due to nativ umip implementaion (the order of running mip6d and radvd daemons),
   * (same for running HA in foreground).
   */


  QuaggaHelper quagga_helper;
  quagga_helper.ra_interval = ra_interval; // quagga-helper is modified (a little :) 
                                           // in order to allow change of routing advertisement interval from the script
  // AR1
  quagga_helper.EnableRadvd (ar1, "sim1", "2001:5::/64");
  quagga_helper.EnableHomeAgentFlag (ar1, "sim1"); 
  // AR2
  quagga_helper.EnableRadvd (ar2, "sim1", "2001:6::/64"); 
  // AR3
  quagga_helper.EnableRadvd (ar3, "sim1", "2001:7::/64");
  // HA  
  quagga_helper.EnableRadvd (ha, "sim0",  "2001:5::/64"); 
  quagga_helper.EnableHomeAgentFlag (ha, "sim0"); 

  // IR  
  quagga_helper.EnableRadvd (ir, "sim0",  "2001:1::/64"); 
  //quagga_helper.EnableHomeAgentFlag (ir, "sim0"); // if the H-bit is set ND doesn't work in ns-3 node in that subnet

  quagga_helper.EnableZebraDebug (ar_nodes);
  quagga_helper.Install (ar_nodes);
  quagga_helper.EnableZebraDebug (ha_nodes);
  quagga_helper.Install (ha_nodes);
  quagga_helper.EnableZebraDebug (ir_nodes);
  quagga_helper.Install (ir_nodes);



//  ****Applications


  startAppTime=initializationTime;
  stopAppTime=initializationTime + walkingTime;
  endSimulationTime=stopAppTime + 0.5;
  //std::cout << "startAppTime = " << startAppTime << std::endl;
  //std::cout << "stopAppTime = " << stopAppTime << std::endl;

//  Ping6 Application
  if (trafficType == "PING")
    {
      // CN->MN
      DceApplicationHelper dce_1;
      dce_1.SetBinary ("ping6");
      dce_1.SetStackSize (1 << 16);
      dce_1.ResetArguments ();
      dce_1.ResetEnvironment ();
      dce_1.AddArgument ("2001:5::200:ff:fe00:202");
      ApplicationContainer apps_1 = dce_1.Install (cn);
      apps_1.Start (Seconds (startAppTime));
    }

// UDPCBR app
   if (trafficType == "UDPCBR")
     {
      DataRate R("2.5Mib/s");
      int payloadSize = 512;

      uint16_t port=1234;      
      OnOffHelper onoffhelper("ns3::UdpSocketFactory", Address (Inet6SocketAddress("2001:5::200:ff:fe00:202", port)));
      onoffhelper.SetAttribute("PacketSize", UintegerValue(payloadSize));                                 
      onoffhelper.SetAttribute("DataRate", DataRateValue(R));       
      onoffhelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
      onoffhelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

      ApplicationContainer onoffApp=onoffhelper.Install(cn_nodes.Get(0));
      onoffApp.Start(Seconds(startAppTime));
      onoffApp.Stop(Seconds(stopAppTime));
     } 

// TCPCBR app
   if (trafficType == "TCPCBR") // It'll wait until dce 1.3 release, MN doesn't listen on TCP port and the connection is not established
     {
      DataRate R("2.5Mib/s");
      int payloadSize = 512;

      uint16_t port=9;      
      OnOffHelper onoffhelper("ns3::TcpSocketFactory", Address (Inet6SocketAddress("2001:5::200:ff:fe00:202", port)));
      onoffhelper.SetAttribute("PacketSize", UintegerValue(payloadSize));                                 
      onoffhelper.SetAttribute("DataRate", DataRateValue(R));       
      onoffhelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
      onoffhelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

      ApplicationContainer onoffApp=onoffhelper.Install(cn_nodes.Get(0));
      onoffApp.Start(Seconds(startAppTime));
      onoffApp.Stop(Seconds(stopAppTime));

      /*
      // TCP sinc in MN
      //PacketSinkHelper sinkhelpertcp6("ns3::LinuxTcp6SocketFactory", Address (Inet6SocketAddress(Ipv6Address::GetAny(), port)));
      PacketSinkHelper sinkhelpertcp6 = PacketSinkHelper ("ns3::LinuxTcp6SocketFactory",
                                            Inet6SocketAddress (Ipv6Address::GetAny (), 9));
      sinkhelpertcp6.SetAttribute ("Local", AddressValue (Inet6SocketAddress (Ipv6Address::GetAny (), 9)));
      ApplicationContainer sinkApptcp6=sinkhelpertcp6.Install(cn_nodes.Get(0));
      sinkApptcp6.Start(Seconds(startAppTime));
      sinkApptcp6.Stop(Seconds(stopAppTime));
      */
     } 

// VoIP app
  if (trafficType == "VOIP")  //OnOffApplication
    {
      DataRate R("68800");  // for packetization time 20ms and G711 Codec output = 64Kbps => payload packet size = 160 bytes
                            // 50 packets/s x 172bytes (160 payload + 12 RTP) = 50 x 172x8 = 68800 bps
                            // Mib=1024Ki=1024*1024b=1048576b   DataRateValue("10Mib/s")
      uint32_t payloadSize = 172;  // [bytes]
      //stats.SetPacketSize(payloadSize);

      uint16_t port=1234;
      OnOffHelper onoffhelper("ns3::UdpSocketFactory", Address (Inet6SocketAddress("2001:5::200:ff:fe00:202", port)));
      onoffhelper.SetAttribute("PacketSize", UintegerValue(payloadSize));                                 
      onoffhelper.SetAttribute("DataRate", DataRateValue(R));  
      onoffhelper.SetAttribute("OnTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.352]"));
      onoffhelper.SetAttribute("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.65]"));  

      ApplicationContainer onoffApp=onoffhelper.Install(cn_nodes.Get(0));
      onoffApp.Start(Seconds(startAppTime));
      onoffApp.Stop(Seconds(stopAppTime));
    }

// Video streaming app
  if (trafficType == "VIDEO_S") // streaming mpeg4 video (over udp)
    { 
      uint32_t MaxPacketSize = 1412;  // Back off 20 (IP) (+++ 60 IP) + 8 (UDP) bytes from MTU  
      uint16_t port=49153;
      UdpTraceClientHelper client (Ipv6Address("2001:5::200:ff:fe00:202"), port,
                                   "/root/workspace/bake/source/formula1_medium_quality.dat");
      client.SetAttribute ("MaxPacketSize", UintegerValue (MaxPacketSize));
      ApplicationContainer apps = client.Install (cn);
      apps.Start (Seconds (startAppTime));
      apps.Stop (Seconds (stopAppTime));
    }


/**** background Nodes
*
* Inserts same number of nodes in Foreighn2 and Foreighn3 networks witch communicates between eachother
* to introduce background traffic and influence the MIPv6 performances in MN.
* MN moves from FN2 to FN3.
*
* Note!!!  If the H bit is set in the RAs ND between ns3-node and AR fails; if H-bit is unset the MIPv6 performances are vastly degraded
* waits for dce 1.3 to see if it'll pass with linux background nodes????????? 
*
**/
  
  if (background_nodes == true)
    {
      if (mn_movement ==1 or mn_path !=2) 
        {std::cout << "background Nodes scenario implemented for MN moovement between FN2-FN3!" << std::endl;}

      // create nodes and devices - declare globally
      //int b_N = 1; // = bN;  //number of background nodes, bn-nodes in HA and bn-nodes in FN2.
      bNodes2.Create (bN);
      bNodes3.Create (bN); 

      // initialize devices
      wifi_bck_devices2 = wifi.Install (phy, mac, bNodes2);    // ex. bn1 addresses 11, 12, 13..
      wifi_bck_devices3 = wifi.Install (phy, mac, bNodes3);    // bn2 addresses 14, 15, 16..

      // mobility
      MobilityHelper d_mobility;
      mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", StringValue ("300.0"),
                                     "Y", StringValue ("100.0"),
                                     "Rho", StringValue ("ns3::UniformRandomVariable[Min=20|Max=50]"));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (bNodes2);

      mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", StringValue ("500.0"),
                                     "Y", StringValue ("100.0"),
                                     "Rho", StringValue ("ns3::UniformRandomVariable[Min=20|Max=50]"));
      mobility.Install (bNodes3);



      // Internet stack
      InternetStackHelper b_stack;
      b_stack.SetIpv4StackInstall (false);
      b_stack.Install (bNodes2);
      b_stack.Install (bNodes3);
      // IPv6 autoconfiguration in ns3????
 
      // b1
      Ipv6AddressHelper ipv6_add_helperb2;
      ipv6_add_helperb2.SetBase (Ipv6Address ("2001:6::"), 64, Ipv6Address("::11")); // 11 is addres index in hex = 17dec
      Ipv6InterfaceContainer b2_interfaces;
      Ipv6StaticRoutingHelper routingHelper_b2;
      Ptr<Ipv6StaticRouting> bn2_routing;
      for (int i = 0; i < bN; i++) 
        {
 	  b2_interfaces = ipv6_add_helperb2.Assign (wifi_bck_devices2.Get (i));

	  bn2_routing  = routingHelper_b2.GetStaticRouting (bNodes2.Get (i)->GetObject<Ipv6> ());
      	  bn2_routing->SetDefaultRoute(Ipv6Address ("2001:6::200:ff:fe00:e"), 1, Ipv6Address ("::"), 0);
				}


      	  //b2

          Ipv6AddressHelper ipv6_add_helperb3;

          int ii = 17 + bN; 
	  std::ostringstream oss; oss << std::hex << ii; // if bN=1 this should produce an index of 12
	  std::string addr3index = oss.str();
          std::string addr3 = "2001:7::200:ff:fe00:" + addr3index;

          ipv6_add_helperb3.SetBase (Ipv6Address ("2001:7::"), 64, Ipv6Address(addr3.c_str ()));
          Ipv6InterfaceContainer b3_interfaces;
          Ipv6StaticRoutingHelper routingHelper_b3;
          Ptr<Ipv6StaticRouting> bn3_routing;
	    
            for (int i = 0; i < bN; i++) 
              {
 	        b3_interfaces = ipv6_add_helperb3.Assign (wifi_bck_devices3.Get (i));

		bn3_routing  = routingHelper_b3.GetStaticRouting (bNodes3.Get (i)->GetObject<Ipv6> ());
      		bn3_routing->SetDefaultRoute(Ipv6Address ("2001:7::200:ff:fe00:f"), 1, Ipv6Address ("::"), 0);
	      }



          // applications; ex. bn2_1 -> bn3_1;  bn2_2 -> bn3_2;  bn2_3 -> bn3_3 ..
    
          DataRate R("0.1Mib/s");  
          uint32_t payloadSize = 172; 
          uint16_t port=1235;
	
          for (int i = 0; i < bN; i++)
	    {
      	      OnOffHelper onoffhelperb("ns3::UdpSocketFactory", Address (Inet6SocketAddress(addr3.c_str (), port)));
      	      onoffhelperb.SetAttribute("PacketSize", UintegerValue(payloadSize));                                 
      	      onoffhelperb.SetAttribute("DataRate", DataRateValue(R));  
      	      //onoffhelperb.SetAttribute("OnTime",  StringValue ("ns3::ExponentialRandomVariable[Mean=0.352]"));
      	      //onoffhelperb.SetAttribute("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.65]"));
      	      onoffhelperb.SetAttribute("OnTime",  StringValue ("ns3::UniformRandomVariable[Min=0.1,Max=0.3]"));
      	      onoffhelperb.SetAttribute("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.4,Max=0.6]"));
     	      ApplicationContainer onoffAppb = onoffhelperb.Install(bNodes2.Get (i));
      	      onoffAppb.Start(Seconds(startAppTime));
      	      onoffAppb.Stop(Seconds(stopAppTime));

              // next destinaton address; maby there is some more elegant way; I came up with this :-) 
              oss.str("");
	      addr3index.clear ();
	      addr3.clear ();
              ii = ii + 1;
              oss << std::hex << ii;
	      addr3index = oss.str();
              addr3 = "2001:7::200:ff:fe00:" + addr3index;

    	      //Sink Application
    	      PacketSinkHelper sinkhelperb("ns3::UdpSocketFactory", Address (Inet6SocketAddress(Ipv6Address::GetAny(), port)));
    	      ApplicationContainer sinkAppb=sinkhelperb.Install(bNodes3.Get(i));
    	      sinkAppb.Start(Seconds(startAppTime));
    	      sinkAppb.Stop(Seconds(stopAppTime));
        }
    
    }



// ****Tracing

// Callbacks
if (callbacks_enable)
{
  // sent packets
  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/MacTx", MakeCallback(&TxCallback));
  // received packets  
  if (trafficType == "PING")
    {Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/MacRx", MakeCallback(&RxCallback));}
  if (trafficType != "PING")
    {Config::ConnectWithoutContext("NodeList/7/DeviceList/0/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback(&RxCallback));}
  // calculating Hendover delay
  Config::ConnectWithoutContext("NodeList/7/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback(&RxControlCallback)); 
  // x position callback
  // Config::ConnectWithoutContext("NodeList/7/$ns3::MobilityModel/CourseChange", MakeCallback(&XPositionCallback)); 

  // printing performances
  Simulator::Schedule(Seconds(endSimulationTime - 0.5), &Tpa::PrintTrafficPerformances, &stats);
  if (print_throughput){
  Simulator::Schedule(Seconds(endSimulationTime - 0.4), &Tpa::PrintThroughput, &stats);}

}
// pcap files

  if (pcap_enable)
  	{
  		csma.EnablePcap ("a_CN_sim0.pcap",   l1_devices.Get (0), true, true);

  		csma.EnablePcap ("a_IR_sim0.pcap",   l1_devices.Get (1), true, true);
  		csma.EnablePcap ("a_IR_sim1.pcap",   l2_devices.Get (0), true, true);
  		csma.EnablePcap ("a_IR_sim2.pcap",   l3_devices.Get (0), true, true);
  		csma.EnablePcap ("a_IR_sim3.pcap",   l4_devices.Get (0), true, true);

  		csma.EnablePcap ("a_HA_sim0.pcap",   l6_devices.Get (0), true, true);

  		csma.EnablePcap ("a_AP1_sim0.pcap",  l5_devices.Get (1), true, true);
  		csma.EnablePcap ("a_AP1_sim1.pcap",  l6_devices.Get (1), true, true);
  		phy.EnablePcap  ("a_AP1_sim2.pcap",  wifi_ap_devices.Get (0), true, true);

  		csma.EnablePcap ("a_AR1_sim0.pcap",  l2_devices.Get (1), true, true);
  		csma.EnablePcap ("a_AR1_sim1.pcap",  l5_devices.Get (0), true, true);  

  		csma.EnablePcap ("a_AR2_sim0.pcap",  l3_devices.Get (1), true, true);  
  		phy.EnablePcap  ("a_AR2_sim1.pcap",  wifi_ap_devices.Get (1), true, true);

  		csma.EnablePcap ("a_AR3_sim0.pcap",  l4_devices.Get (1), true, true);  
  		phy.EnablePcap  ("a_AR3_sim1.pcap",  wifi_ap_devices.Get (2), true, true);

  		phy.EnablePcap  ("a_MN_sim0.pcap",   wifi_sta_devices.Get (0), true, true);

  		//background
  		if (background_nodes == true)
    		{
      		phy.EnablePcap  ("a_BN2_1sim0.pcap",  wifi_bck_devices2.Get (0), true, true); 
      		phy.EnablePcap  ("a_BN3_1sim0.pcap",  wifi_bck_devices3.Get (0), true, true);
    		}
		}


// ****Running the script
  Simulator::Stop (Seconds (endSimulationTime));  //if this time is small the pcap files will be empty 0B

//*** NetAnim
	if (anim_enable)
    {
    	AnimationInterface::SetNodeDescription (cn, "CN");
    	AnimationInterface::SetNodeDescription (ir, "IR");
    	AnimationInterface::SetNodeDescription (ha, "HA");
    	AnimationInterface::SetNodeDescription (ap1, "AP1");
    	AnimationInterface::SetNodeDescription (ar1, "AR1");
    	AnimationInterface::SetNodeDescription (ar2, "AP2");
    	AnimationInterface::SetNodeDescription (ar3, "AP3");
    	AnimationInterface::SetNodeDescription (mn, "MN");

      if (background_nodes == true)
        {
        	for (int b_nn = 0; b_nn < bN; b_nn++)
        		{
        		  AnimationInterface::SetNodeDescription (bNodes2.Get (b_nn), "bn1");
        		  AnimationInterface::SetNodeDescription (bNodes3.Get (b_nn), "bn2");
        		}
      	}

    	AnimationInterface anim ("a_mipv6anim.xml");
    	anim.SetMobilityPollInterval(Seconds(0.1));
    	//anim.EnablePacketMetadata(true);
		}
// Config store
  if (0){ 
  GtkConfigStore config;
  config.ConfigureDefaults();
  config.ConfigureAttributes();}

// Run simulation

  Simulator::Run();
  Simulator::Destroy();

return 0;
}
