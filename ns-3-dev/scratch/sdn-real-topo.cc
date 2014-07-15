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
#include "ns3/random-variable.h"

#include <string>
#include <fstream>
#include <stdlib.h>
#include <string>

#include "sdn-real-topo.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AndrewSDNTopologyTest");

/**
 * Initialize the connectivity graph from the given topology file.
 */
void InitializeTopology (std::string filename)
{
  NS_LOG_INFO("* Initializing topology from \"" << filename << "\"");
  std::cerr << "* Initializing topology from \"" << filename << "\"\n";
  std::map<uint32_t, std::list<uint32_t>*> nodeMapping;
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
  for (uint32_t i = 0; i < numNodes; i++) {
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
      NS_LOG_LOGIC ("  " << from << " <-> " << to);
    }
  }
  std::cerr << "* Topology initialization complete!\n";
}

void RecomputeDDC () {
  GlobalRouteManager::DeleteGlobalRoutes ();
  GlobalRouteManager::BuildGlobalRoutingDatabase ();
  GlobalRouteManager::InitializeRoutes ();
  Simulator::Schedule (Seconds (ddcRefreshInterval), &RecomputeDDC);
}

void RecoverLink (PointToPointChannel* link) {
  uint32_t node1 = link->GetDevice (0)->GetNode ()->GetId ();
  uint32_t node2 = link->GetDevice (1)->GetNode ()->GetId ();
  NS_LOG_INFO ("Recovering link " << node1 << " <-> " << node2);
  link->SetLinkUp ();
  channels.push_back (link);
  numLinksToFail++;
}

/**
 * Fail a random link.
 */
void FailRandomLink () {
  if (channels.size () > 0 && numLinksToFail > 0) {
    uint32_t linkIndex = rv.GetInteger (0, channels.size () - 1);
    PointToPointChannel* linkToFail = channels.at (linkIndex);
    uint32_t node1 = linkToFail->GetDevice (0)->GetNode ()->GetId ();
    uint32_t node2 = linkToFail->GetDevice (1)->GetNode ()->GetId ();
    NS_LOG_INFO ("Failing link " << node1 << " <-> " << node2);
    linkToFail->SetLinkDown ();
    channels.erase (channels.begin () + linkIndex);
    numLinksToFail--;

    // Schedule next failure
    uint32_t failureDelay = rv.GetInteger (minFailureDelay, maxFailureDelay);
    Simulator::Schedule (Seconds (failureDelay), &FailRandomLink);

    // Schedule recovery
    uint32_t recoveryDelay = rv.GetInteger (minRecoveryDelay, maxRecoveryDelay);
    Simulator::Schedule (Seconds (recoveryDelay), &RecoverLink, linkToFail);

  } else {
    NS_LOG_LOGIC ("No more links to fail!");
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

  if (argc < 6) {
    NS_LOG_ERROR ("Usage: sdn-real-topo " <<
                    "[topology file] " <<
                    "[controller id start] " <<
                    "[controller id end] " <<
                    "[links to fail] " <<
                    "[seed]" <<
                    "[reverse delay]" <<
                    "[experiment name]");
    exit (EXIT_FAILURE);
  }

  InitializeTopology (argv[1]);

  // Parse command line arguments
  uint32_t oldControllerStartID = std::atoi (argv[2]);
  uint32_t oldControllerEndID = std::atoi (argv[3]);
  if (nodeTranslateMap.find (oldControllerStartID) == nodeTranslateMap.end ()) {
    NS_LOG_ERROR ("Controller start ID " << oldControllerStartID << " not found!");
    exit (EXIT_FAILURE);
  }
  if (nodeTranslateMap.find (oldControllerEndID) == nodeTranslateMap.end ()) {
    NS_LOG_ERROR ("Controller end ID " << oldControllerEndID << " not found!");
    exit (EXIT_FAILURE);
  }
  if (oldControllerStartID > oldControllerEndID) {
    NS_LOG_ERROR (
      "Controller start ID " << oldControllerStartID << " must be <= " <<
      "controller end ID " << oldControllerEndID << "!");
    exit (EXIT_FAILURE);
  }
  uint32_t controllerStartID = nodeTranslateMap.at (std::atoi (argv[2]));
  uint32_t controllerEndID = nodeTranslateMap.at (std::atoi (argv[3]));
  NS_LOG_LOGIC ("Controllers ID range (new IDs): "
    << controllerStartID << " - " << controllerEndID);
  numLinksToFail = std::atoi (argv[4]);
  uint32_t seed = std::atoi (argv[5]);
  SeedManager::SetSeed (seed);
  double reverseDelay = std::atof (argv[6]);
  std::string expName = argv[7];

  std::cerr << "* Setting nodes up\n";

  // Initialize the nodes
  NodeContainer nodes;
  nodes.Create (numNodes);

  // Connect the nodes
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (10)));
  p2p.SetQueue ("ns3::PriorityQueue");
  std::vector<NetDeviceContainer> nodeDevices (numNodes);  
  std::vector<NetDeviceContainer> linkDevices;
  for (uint32_t i = 0; i < numNodes; i++) {
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

  // Schedule link failures
  if (numLinksToFail > channels.size ()) {
    NS_LOG_ERROR ("Attempted to fail more links " <<
      "(" << numLinksToFail << ") than possible " <<
      "(" << channels.size () << ")!");
    exit (EXIT_FAILURE);
  }
  NS_LOG_INFO ("* Failing " << numLinksToFail << " links over the course of the simulation.");
  uint32_t failureDelay = rv.GetInteger (minFailureDelay, maxFailureDelay);
  Simulator::Schedule (Seconds (failureDelay), &FailRandomLink);

  std::cerr << "* Installing Ipv4 stack on each node\n";

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

  // Set reversal delays for each node
  for (uint32_t i = 0; i < numNodes; i++) {
    Ptr<GlobalRouter> router = nodes.Get(i)->GetObject<GlobalRouter>();
    Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol();
    gr->SetAttribute("ReverseOutputToInputDelay", TimeValue (MicroSeconds (reverseDelay)));
    gr->SetAttribute("ReverseInputToOutputDelay", TimeValue (MicroSeconds (reverseDelay)));
  }

  std::cerr << "* Setting up switches and controllers\n";

  // Set up switches and controllers
  NodeContainer switchNodes;
  NodeContainer controllerNodes;
  for (uint32_t i = 0; i < numNodes; i++) {
    if (i >= controllerStartID && i <= controllerEndID) {
      controllerNodes.Add (nodes.Get (i));
    } else {
      switchNodes.Add (nodes.Get (i));
    }
  }

  std::cerr << "* Installing switch and controller applications\n";

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
      controllerMaxEpoch,
      expName);
  ApplicationContainer switchApps = switchHelper->Install (switchNodes, 1001);
  ApplicationContainer controllerApps = controllerHelper->Install (controllerNodes, 1);

  std::cerr << "* Connecting controllers to switches\n";

  controllerHelper->ConnectToSwitches (controllerNodes, switchNodes);

  // On exit, close all open files (hack)
  std::list<std::ofstream*> filesToClose;
  ApplicationContainer::Iterator it;
  for (it = controllerApps.Begin (); it != controllerApps.End (); it++) {
    SimpleSDNController* app = (SimpleSDNController*) GetPointer (*it);
    filesToClose.push_back(app->GetFile ());
  }
  for (it = controllerApps.Begin (); it != controllerApps.End (); it++) {
    SimpleSDNController* app = (SimpleSDNController*) GetPointer (*it);
    app->SetFilesToClose (filesToClose);
  }

  // In case links go back up, re-initialize DDC periodically
  // NOTE: This currently causes a DDC assertion failure!
  //Simulator::Schedule (Seconds (ddcRefreshInterval), &RecomputeDDC);

  // Actually start the simulation
  std::cerr << "-- Simulation starting --\n";
  NS_LOG_INFO ("-- Simulation starting --");
  Simulator::Run ();

  // Clean up
  Simulator::Destroy ();
  for (uint32_t i = 0; i < numNodes; i++) {
    delete connectivityGraph[i];
  }

  NS_LOG_INFO ("-- Simulation complete --");
  return 0;
}

