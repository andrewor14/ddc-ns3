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

#include "sdn-test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AndrewSDNTest");

/**
 * Initialize the connectivity graph.
 */
void InitializeTopology ()
{
  for (int i = 0; i < numNodes; i++) {
    connectivityGraph[i] = new std::list<uint32_t>;
  }
  connectivityGraph[0]->push_back (1);
  connectivityGraph[0]->push_back (5);
  connectivityGraph[0]->push_back (6);
  connectivityGraph[0]->push_back (7);

  connectivityGraph[1]->push_back (6);
  connectivityGraph[1]->push_back (7);
  connectivityGraph[1]->push_back (8);

  connectivityGraph[2]->push_back (3);
  connectivityGraph[2]->push_back (5);
  connectivityGraph[2]->push_back (7);
  connectivityGraph[2]->push_back (8);
  connectivityGraph[2]->push_back (9);

  connectivityGraph[3]->push_back (8);
  connectivityGraph[3]->push_back (9);
  connectivityGraph[3]->push_back (10);

  connectivityGraph[4]->push_back (5);
  connectivityGraph[4]->push_back (9);
  connectivityGraph[4]->push_back (10);
  connectivityGraph[4]->push_back (11);

  connectivityGraph[7]->push_back (8);
  connectivityGraph[8]->push_back (11);
  connectivityGraph[9]->push_back (10);
  connectivityGraph[10]->push_back (11);
}

/**
 * Return the InetSocketAddress of a node.
 */
InetSocketAddress* GetInetSocketAddress (Ptr<Node> node, bool isController) {
  Address addr = node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  Ipv4Address ipv4Addr = Ipv4Address::ConvertFrom (addr);
  if (isController) {
    return new InetSocketAddress (ipv4Addr, controllerPort);
  } else {
    return new InetSocketAddress (ipv4Addr, switchPort);
  }
}

/**
 * Run the simulation.
 */
int main (int argc, char *argv[])
{
  LogComponentEnable ("AndrewSDNTest", LOG_LEVEL_INFO);
  LogComponentEnable ("Ipv4GlobalRouting", LOG_LEVEL_WARN);
  LogComponentEnable ("SimpleSDNControllerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("SimpleSDNSwitchApplication", LOG_LEVEL_INFO);

  InitializeTopology ();

  // Initialize the nodes
  NodeContainer nodes;
  nodes.Create (numNodes);

  // Connect the nodes
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  p2p.SetQueue ("ns3::PriorityQueue");
  std::vector<NetDeviceContainer> nodeDevices (numNodes);  
  std::vector<NetDeviceContainer> linkDevices;
  std::vector<PointToPointChannel*> channels;
  for (int i = 0; i < numNodes; i++) {
    for (std::list<uint32_t>::iterator it = connectivityGraph[i]->begin ();
         it != connectivityGraph[i]->end ();
         it++) {
      NetDeviceContainer p2pDevices = p2p.Install (nodes.Get (i), nodes.Get (*it));
      nodeDevices[i].Add (p2pDevices.Get (0));
      nodeDevices[*it].Add (p2pDevices.Get (1));
      linkDevices.push_back (p2pDevices);
      Channel* channel = GetPointer (p2pDevices.Get (0)->GetChannel ());
      channels.push_back ((PointToPointChannel*) channel);
    }
  }

  // Set up each network device
  InternetStackHelper stack;
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  for (int i = 0; i < (int) linkDevices.size (); i++) {
    Ipv4InterfaceContainer current = address.Assign (linkDevices[i]);
    address.NewNetwork ();
  }
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Set up switches
  SimpleSDNSwitchHelper* switchHelper =
    new SimpleSDNSwitchHelper (
      switchPort,
      switchWindowDuration,
      switchMaxViolationCount);
  ApplicationContainer switchApp1 = switchHelper->Install (nodes.Get (0), 1001);
  InetSocketAddress* switchAddress1 = GetInetSocketAddress (nodes.Get (0), false);

  // Set up controllers
  SimpleSDNControllerHelper* controllerHelper =
    new SimpleSDNControllerHelper (
      controllerPort,
      controllerPingSwitchesInterval,
      controllerPingControllersInterval,
      controllerMaxEpoch);
  ApplicationContainer controllerApp1 = controllerHelper->Install (nodes.Get (1), 1);
  ApplicationContainer controllerApp2 = controllerHelper->Install (nodes.Get (2), 2);
  ApplicationContainer controllerApp3 = controllerHelper->Install (nodes.Get (3), 3);
  ApplicationContainer controllerApp4 = controllerHelper->Install (nodes.Get (4), 4);
  SimpleSDNController* controller1 = (SimpleSDNController*) GetPointer (controllerApp1.Get (0));
  SimpleSDNController* controller2 = (SimpleSDNController*) GetPointer (controllerApp2.Get (0));
  SimpleSDNController* controller3 = (SimpleSDNController*) GetPointer (controllerApp3.Get (0));
  SimpleSDNController* controller4 = (SimpleSDNController*) GetPointer (controllerApp4.Get (0));
  InetSocketAddress* controllerAddress1 = GetInetSocketAddress (nodes.Get (1), true);
  InetSocketAddress* controllerAddress2 = GetInetSocketAddress (nodes.Get (2), true);
  InetSocketAddress* controllerAddress3 = GetInetSocketAddress (nodes.Get (3), true);
  InetSocketAddress* controllerAddress4 = GetInetSocketAddress (nodes.Get (4), true);

  // Connect controllers to each other
  controller1->AddPeeringController (*controllerAddress2);
  controller1->AddPeeringController (*controllerAddress3);
  controller1->AddPeeringController (*controllerAddress4);
  controller2->AddPeeringController (*controllerAddress1);
  controller2->AddPeeringController (*controllerAddress3);
  controller2->AddPeeringController (*controllerAddress4);
  controller3->AddPeeringController (*controllerAddress1);
  controller3->AddPeeringController (*controllerAddress2);
  controller3->AddPeeringController (*controllerAddress4);
  controller4->AddPeeringController (*controllerAddress1);
  controller4->AddPeeringController (*controllerAddress2);
  controller4->AddPeeringController (*controllerAddress3);

  // Connect controllers to switches
  controller1->AddPeeringSwitch (*switchAddress1);
  controller2->AddPeeringSwitch (*switchAddress1);
  controller3->AddPeeringSwitch (*switchAddress1);
  controller4->AddPeeringSwitch (*switchAddress1);

  // Actually start the simulation
  NS_LOG_INFO ("-- Simulation starting --");
  Simulator::Run ();

  // Clean up
  Simulator::Destroy ();
  for (int i = 0; i < numNodes; i++) {
    delete connectivityGraph[i];
  }

  NS_LOG_INFO ("-- Simulation complete --");
  return 0;
}

