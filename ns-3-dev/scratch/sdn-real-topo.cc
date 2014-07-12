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

#include <string>
#include <stdlib.h>

#include "sdn-real-topo.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AndrewSDNTopologyTest");

/**
 * Initialize the connectivity graph from the given topology file.
 */
void InitializeTopology (std::string filename)
{
  NS_LOG_INFO ("* Initializing topology from \"" << filename << "\"");
  std::map<uint32_t, std::list<uint32_t>*> nodeMapping;
  std::map<uint32_t, uint32_t> nodeTranslateMap;
  std::ifstream file (filename.c_str ());
  if (!file.is_open ()) {
    NS_LOG_ERROR("File " << filename << " not found or cannot be opened!");
    exit (EXIT_FAILURE);
  }

  // Populate node connectivity mapping from file
  while (file.good ()) {
    std::string input;
    getline (file, input);
    size_t found = input.find (" ");
    if (found == std::string::npos) {
      continue;
    }
    std::string node1s = input.substr (0, int (found)).c_str ();
    std::string node2s =
      input.substr (int (found) + 1, input.length () - (int (found) + 1));
    uint32_t node1 = std::atoi (node1s.c_str ()) ;
    uint32_t node2 = std::atoi (node2s.c_str ());
    if (!nodeMapping[node1]) { nodeMapping[node1] = new std::list<uint32_t>; }
    if (!nodeMapping[node2]) { nodeMapping[node2] = new std::list<uint32_t>; }
    nodeMapping[node1]->push_back(node2);
  }

  numNodes = nodeMapping.size();
  connectivityGraph.resize (numNodes);

  // Translate old ID to new ID in case the ID space is not continuous
  uint32_t new_id = 0;
  std::map<uint32_t, std::list<uint32_t>*>::iterator it;
  NS_LOG_LOGIC ("Initializing node mapping");
  for (it = nodeMapping.begin (); it != nodeMapping.end (); it++) {
    uint32_t old_id = it->first;
    NS_LOG_LOGIC ("  " << old_id << " -> " << new_id);
    nodeTranslateMap[old_id] = new_id++;
  }

  // Initialize connectivity graph from node mapping
  for (int i = 0; i < numNodes; i++) {
    connectivityGraph.at (i) = new std::list<uint32_t>;
  }
  NS_LOG_LOGIC ("Adding bi-directional connections (using new node IDs)");
  for (it = nodeMapping.begin (); it != nodeMapping.end (); it++) {
    uint32_t from = nodeTranslateMap[it->first];
    std::list<uint32_t>* neighbors = it->second;
    std::list<uint32_t>::iterator it_n;
    for (it_n = neighbors->begin (); it_n != neighbors->end (); it_n++) {
      uint32_t to = nodeTranslateMap[*it_n];
      connectivityGraph.at (from)->push_back (to);
      NS_LOG_LOGIC("  " << from << " <-> " << to);
    }
  }
}

/**
 * Run the simulation.
 */
int main (int argc, char *argv[])
{
  LogComponentEnable ("AndrewSDNTopologyTest", LOG_LEVEL_INFO);
  LogComponentEnable ("Ipv4GlobalRouting", LOG_LEVEL_WARN);
  LogComponentEnable ("SimpleSDNControllerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("SimpleSDNSwitchApplication", LOG_LEVEL_INFO);

  if (argc < 2) {
    NS_LOG_ERROR("Usage: sdn-real-topo [topology file]");
    exit (EXIT_FAILURE);
  }

  InitializeTopology (argv[1]);

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

  // Set up switches and controllers
  NodeContainer switchNodes;
  NodeContainer controllerNodes;
  for (int i = 0; i < numNodes; i++) {
    if (i >= 1 && i <= 4) {
      controllerNodes.Add (nodes.Get (i));
    } else {
      switchNodes.Add (nodes.Get (i));
    }
  }

  // Install switch and controller applications
  SimpleSDNSwitchHelper* switchHelper =
    new SimpleSDNSwitchHelper (
      switchPort,
      switchWindowDuration,
      switchMaxViolationCount);
  SimpleSDNControllerHelper* controllerHelper =
    new SimpleSDNControllerHelper (
      controllerPort,
      controllerPingSwitchesInterval,
      controllerPingControllersInterval,
      controllerMaxEpoch);
  ApplicationContainer switchApps = switchHelper->Install (switchNodes, 1001);
  ApplicationContainer controllerApps = controllerHelper->Install (controllerNodes, 1);
  controllerHelper->ConnectToSwitches (controllerNodes, switchNodes);

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

