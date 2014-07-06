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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <list>
#include <vector>
#include <stack>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Andrew-DDC-test");

// Number of nodes in this network
const int32_t numNodes = 12;
// Graph representation of the network
std::list<uint32_t>* connectivityGraph [numNodes];
// Server port
const uint32_t serverPort = 7070;
// Simulation end time
Time simulationEnd = Seconds(60.0 * 60.0 * 24 * 7);


/**
 * Initialize the connectivity graph.
 */
void InitializeTopology()
{
  for (int i = 0; i < numNodes; i++) {
    connectivityGraph[i] = new std::list<uint32_t>;
  }
  connectivityGraph[0]->push_back(1);
  connectivityGraph[0]->push_back(5);
  connectivityGraph[0]->push_back(6);
  connectivityGraph[0]->push_back(7);

  connectivityGraph[1]->push_back(6);
  connectivityGraph[1]->push_back(7);
  connectivityGraph[1]->push_back(8);

  connectivityGraph[2]->push_back(3);
  connectivityGraph[2]->push_back(5);
  connectivityGraph[2]->push_back(7);
  connectivityGraph[2]->push_back(8);
  connectivityGraph[2]->push_back(9);

  connectivityGraph[3]->push_back(8);
  connectivityGraph[3]->push_back(9);
  connectivityGraph[3]->push_back(10);

  connectivityGraph[4]->push_back(5);
  connectivityGraph[4]->push_back(9);
  connectivityGraph[4]->push_back(10);
  connectivityGraph[4]->push_back(11);

  connectivityGraph[7]->push_back(8);
  connectivityGraph[8]->push_back(11);
  connectivityGraph[9]->push_back(10);
  connectivityGraph[10]->push_back(11);
}

/**
 * Callback invoked when a server receives a packet.
 */
void ServerRxPacket(Ptr<const Packet> packet, Ipv4Header &header) {
  NS_LOG_INFO("Server received packet");
}

/**
 * Callback invoked when a client receives a packet.
 */
void ClientRxPacket(Ptr<const Packet> packet, Ipv4Header &header) {
  NS_LOG_INFO("Client received packet");
}

/**
 * Run the simulation.
 */
int main (int argc, char *argv[])
{
  LogComponentEnable ("Andrew-DDC-test", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  InitializeTopology();

  // Initialize the nodes
  NodeContainer nodes;
  nodes.Create(numNodes);

  // Connect the nodes
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  p2p.SetQueue ("ns3::PriorityQueue");
  std::vector<NetDeviceContainer> nodeDevices(numNodes);  
  std::vector<NetDeviceContainer> linkDevices;
  std::vector<PointToPointChannel*> channels;
  for (int i = 0; i < numNodes; i++) {
    for (std::list<uint32_t>::iterator it = connectivityGraph[i]->begin();
         it != connectivityGraph[i]->end();
         it++) {
      NetDeviceContainer p2pDevices = p2p.Install (nodes.Get(i), nodes.Get(*it));
      nodeDevices[i].Add(p2pDevices.Get(0));
      nodeDevices[*it].Add(p2pDevices.Get(1));
      linkDevices.push_back(p2pDevices);
      Channel* channel = GetPointer(p2pDevices.Get(0)->GetChannel());
      channels.push_back((PointToPointChannel*)channel);
    }
  }

  // Set up each network device
  InternetStackHelper stack;
  stack.Install(nodes);
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  for (int i = 0; i < (int) linkDevices.size(); i++) {
    Ipv4InterfaceContainer current = address.Assign(linkDevices[i]);
    address.NewNetwork();
  }
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Set up server and clients
  // For now, server is always the first node (with index 0)
  UdpEchoServerHelper echoServer (serverPort);
  ApplicationContainer serverApps = echoServer.Install (nodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (simulationEnd);
  for (int i = 0; i < numNodes; i++) {
    UdpEchoServer* server = (UdpEchoServer*) GetPointer(serverApps.Get(i));
    server->AddReceivePacketEvent(MakeCallback(&ServerRxPacket));
    Address serverAddress = nodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    UdpEchoClientHelper echoClient (serverAddress, serverPort);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (0));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (3.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = echoClient.Install (nodes.Get (i));
    clientApps.Stop (simulationEnd);
    UdpEchoClient* client = (UdpEchoClient*) GetPointer(clientApps.Get(0));
    client->AddReceivePacketEvent(MakeCallback(&ClientRxPacket));
    if (i == 4) {
      Simulator::Schedule(Seconds(5.0), &UdpEchoClient::SendControlPacket, client);
    }
  }

  // Actually start the simulation
  NS_LOG_INFO("-- Simulation starting --");
  Simulator::Run();

  // Clean up
  Simulator::Destroy();
  for (int i = 0; i < numNodes; i++) {
    delete connectivityGraph[i];
  }

  NS_LOG_INFO("-- Simulation complete --");
  return 0;
}

