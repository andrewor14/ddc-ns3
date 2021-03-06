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
#include "ns3/nstime.h"

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
  std::cerr << "* Initializing topology from \"" << filename << "\"\n";
  NS_LOG_INFO("* Initializing topology from \"" << filename << "\"");
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

/**
 * Return a sampled failure delay.
 */
Time GetFailureDelay () {
  uint32_t mean = meanFailureDelay.GetNanoSeconds ();
  uint32_t sample = rv.GetInteger (mean * 0.9, mean * 1.1);
  return NanoSeconds (sample);
}

/**
 * Return a sampled recovery delay.
 */
Time GetRecoveryDelay () {
  uint32_t mean = meanRecoveryDelay.GetNanoSeconds ();
  uint32_t sample = rv.GetInteger (mean * 0.9, mean * 1.1);
  return NanoSeconds (sample);
}

/**
 * Recover a random link.
 */
void RecoverRandomLink () {
  if (failedLinks.size () > 0) {
    uint32_t linkIndex = rv.GetInteger (0, failedLinks.size () - 1);
    PointToPointChannel* linkToRecover = failedLinks.at (linkIndex);
    uint32_t node1 = linkToRecover->GetDevice (0)->GetNode ()->GetId ();
    uint32_t node2 = linkToRecover->GetDevice (1)->GetNode ()->GetId ();
    NS_LOG_INFO ("Recovering link " << node1 << " <-> " << node2);
    linkToRecover->SetLinkUp ();
    links.push_back (linkToRecover);
    failedLinks.erase (failedLinks.begin () + linkIndex);
    numLinksToFail++;
    SimpleSDNController::failedLinks--;
  } else {
    std::cerr << "No more links to recover!\n";
    NS_LOG_LOGIC ("No more links to recover!");
  }
  Simulator::Schedule (GetRecoveryDelay (), &RecoverRandomLink);
}

/**
 * Fail a random link.
 */
void FailRandomLink () {
  if (links.size () > 0 && numLinksToFail > 0) {
    uint32_t linkIndex = rv.GetInteger (0, links.size () - 1);
    PointToPointChannel* linkToFail = links.at (linkIndex);
    uint32_t node1 = linkToFail->GetDevice (0)->GetNode ()->GetId ();
    uint32_t node2 = linkToFail->GetDevice (1)->GetNode ()->GetId ();
    NS_LOG_INFO ("Failing link " << node1 << " <-> " << node2);
    linkToFail->SetLinkDown ();
    failedLinks.push_back (linkToFail);
    links.erase (links.begin () + linkIndex);
    numLinksToFail--;
    SimpleSDNController::failedLinks++;
  } else {
    std::cerr << "No more links to fail!\n";
    NS_LOG_LOGIC ("No more links to fail!");
  }
  Simulator::Schedule (GetFailureDelay (), &FailRandomLink);
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

  std::string topoFile;
  std::string expName;
  uint32_t numControllers;
  // controllerMaxEpoch
  // switchMaxViolationCount
  // numLinksToFail
  // meanRecoveryDelay
  // meanFailureDelay
  uint32_t seed = 8888;
  Time reversalDelay;
  Time linkLatency;

  CommandLine cmd;
  cmd.AddValue ("TopologyFile", "File of the topology representation", topoFile);
  cmd.AddValue ("ExperimentName", "Name of experiment", expName);
  cmd.AddValue ("NumControllers", "Number of controllers", numControllers);
  cmd.AddValue ("ControllerMaxEpoch", "Max epoch", controllerMaxEpoch);
  cmd.AddValue ("SwitchMaxViolation", "Max violation count", switchMaxViolationCount);
  cmd.AddValue ("NumLinksToFail", "Number of links to fail", numLinksToFail);
  cmd.AddValue ("MeanRecoveryInterval", "Link recovery interval", meanRecoveryDelay);
  cmd.AddValue ("MeanFailureInterval", "Link failure interval", meanFailureDelay);
  cmd.AddValue ("Seed", "Seed for choosing random links", seed);
  cmd.AddValue ("ReversalDelay", "Delay for link reversal", reversalDelay);
  cmd.AddValue ("LinkLatency", "Link latency", linkLatency);
  cmd.Parse (argc, argv);

  std::cerr <<
    "* Parsed arguments:\n" <<
    "    TopologyFile = " << topoFile << "\n" <<
    "    ExperimentName = " << expName << "\n" <<
    "    NumControllers = " << numControllers << "\n" <<
    "    ControllerMaxEpoch = " << controllerMaxEpoch << "\n" <<
    "    SwitchMaxViolation = " << (uint32_t) switchMaxViolationCount << "\n" <<
    "    NumLinksToFail = " << numLinksToFail << "\n" <<
    "    MeanRecoveryInterval = " << meanRecoveryDelay << "\n" <<
    "    MeanFailureInterval = " << meanFailureDelay << "\n" <<
    "    Seed = " << seed << "\n" <<
    "    ReversalDelay = " << reversalDelay << "\n" <<
    "    LinkLatency = " << linkLatency << "\n" <<
    "\n";

  InitializeTopology (topoFile);
  SeedManager::SetSeed (seed);

  // Compute controller IDs
  std::list<uint32_t> controllerIDs;
  std::cerr << "* Controllers IDs (new ID):";
  while (numControllers > 0) {
    uint32_t controllerID = rv.GetInteger (0, numNodes - 1);
    // Do not add duplicates
    std::list<uint32_t>::iterator it = std::find (controllerIDs.begin (), controllerIDs.end (), controllerID);
    if (it == controllerIDs.end ()) {
      controllerIDs.push_back (controllerID);
      numControllers--;
      std::cerr << " " << controllerID;
    }
  }
  std::cerr << "\n";

  // Initialize the nodes
  std::cerr << "* Setting nodes up\n";
  NodeContainer nodes;
  nodes.Create (numNodes);

  // Connect the nodes
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2p.SetChannelAttribute ("Delay", TimeValue (linkLatency));
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
      Channel* link = GetPointer (p2pDevices.Get (0)->GetChannel ());
      links.push_back ((PointToPointChannel*) link);
    }
  }
  std::cerr << "* There are " << nodes.GetN () << " nodes and " << links.size () << " links\n";

  // Schedule link failures (and recovery)
  std::cerr << "* Failing " << numLinksToFail << " links over the course of the simulation.\n";
  NS_LOG_INFO ("* Failing " << numLinksToFail << " links over the course of the simulation.");
  if (numLinksToFail > links.size ()) {
    NS_LOG_ERROR ("Attempted to fail more links " <<
      "(" << numLinksToFail << ") than possible " <<
      "(" << links.size () << ")!");
    exit (EXIT_FAILURE);
  }
  Simulator::Schedule (GetFailureDelay (), &FailRandomLink);
  Simulator::Schedule (GetRecoveryDelay (), &RecoverRandomLink);

  // Set up each network device
  std::cerr << "* Installing Ipv4 stack on each node\n";
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
    gr->SetAttribute("ReverseOutputToInputDelay", TimeValue (reversalDelay));
    gr->SetAttribute("ReverseInputToOutputDelay", TimeValue (reversalDelay));
  }

  // Set up switches and controllers
  std::cerr << "* Setting up switches and controllers\n";
  NodeContainer switchNodes;
  NodeContainer controllerNodes;
  for (uint32_t i = 0; i < numNodes; i++) {
    std::list<uint32_t>::iterator it = std::find (controllerIDs.begin (), controllerIDs.end (), i);
    if (it != controllerIDs.end ()) {
      controllerNodes.Add (nodes.Get (i));
    } else {
      switchNodes.Add (nodes.Get (i));
    }
  }
  std::cerr << "* Installed " << controllerNodes.GetN () << " controllers\n";

  // Install switch and controller applications
  std::cerr << "* Installing switch and controller applications\n";
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
      expName,
      seed);
  ApplicationContainer switchApps = switchHelper->Install (switchNodes, 1001);
  ApplicationContainer controllerApps = controllerHelper->Install (controllerNodes, 1);
  controllerHelper->ConnectToSwitches (controllerNodes, switchNodes);

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

