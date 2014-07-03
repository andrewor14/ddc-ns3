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

#include "boost/algorithm/string.hpp"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/global-route-manager-impl.h"
#include "ns3/data-rate.h"
#include "ns3/ipv4-header.h"

#include <list>
#include <vector>
#include <stack>
#include <algorithm>
#include <iostream>
#include <map>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <functional>

#include "stretch-classes.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DDC-NSDI-TRAFFIC-SIM-TCP");
static const uint32_t PORT = 22;

class TcpFileTransfer : public Object
{
  protected:
    static const uint64_t TOTAL_TX_BYTES = 20000000; //209715200;
    static const uint64_t WRITE_SIZE = 1040;
    uint8_t m_data[WRITE_SIZE];
    Ptr<Node> m_node;
    Ptr<Socket> m_socket;
    Ptr<Node> m_dest;
    uint64_t m_currentBytes;
 public:
    
    void RetransmitCallback (bool fastRetransmit)
    {
      std::cout << m_node->GetId() << "," << (fastRetransmit ? "FR" : "T") << std::endl;
    }
    TcpFileTransfer(Ptr<Node> node, Ptr<Node> dest) {
      for (uint32_t i = 0; i  < (uint32_t)WRITE_SIZE; i++) {
        m_data[i] = i % 256;
      }
      m_dest = dest;
      m_node = node;
    }

    void StartSending () {
      NS_LOG_LOGIC("Start sending");
      m_socket = Socket::CreateSocket(m_node, TcpSocketFactory::GetTypeId ());
      m_socket->TraceConnectWithoutContext("Retransmit", MakeCallback(&TcpFileTransfer::RetransmitCallback, this));
      m_socket->Bind();
      m_socket->Connect (InetSocketAddress (m_dest->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), PORT));
      m_socket->SetSendCallback(MakeCallback(&TcpFileTransfer::WriteUntilBufferFull, this));
      WriteUntilBufferFull (m_socket, m_socket->GetTxAvailable());
    }

    void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace) {
      NS_LOG_LOGIC("Actually sending");
      while (m_currentBytes < TOTAL_TX_BYTES && localSocket->GetTxAvailable () > 0)
      {
        uint64_t left = TOTAL_TX_BYTES - m_currentBytes;
        uint64_t offset = m_currentBytes % WRITE_SIZE;
        uint32_t toWrite = (uint32_t)std::min (WRITE_SIZE - offset, 
                            std::min (left, (uint64_t)localSocket->GetTxAvailable ()));
        int amountSent = localSocket->Send (&m_data[offset], toWrite, 0);
        if (amountSent < 0) {
          return;
        }
        m_currentBytes += (uint64_t)amountSent;
      }
      localSocket->Close ();
    }
};

class Topology : public Object
{
  protected:
    std::vector<UdpEchoClient*> m_clients;
    std::vector<UdpEchoServer*> m_servers;
    std::vector<PointToPointChannel* > m_channels;
    UniformVariable randVar;
    uint32_t m_numNodes;
    std::vector<std::list<uint32_t>*> m_connectivityGraph;
    Time m_simulationEnd;
    std::vector<uint32_t> m_nodeTranslate;
    std::map<std::pair<uint32_t, uint32_t>, PointToPointChannel*> m_channelMap;
    std::map<uint32_t, uint32_t> m_nodeForwardTranslationMap;
    std::map<Ipv4Address, uint32_t> m_addressToNodeMap; 
    NodeContainer m_nodes;
    std::vector<NetDeviceContainer> m_nodeDevices;
    std::vector<NetDeviceContainer> m_linkDevices;
    std::vector<NodeCallback> m_callbacks;
    std::vector<std::pair<uint32_t, uint32_t> > m_pathsToTest;
    uint32_t m_currentPath;
    uint32_t m_currentTrial;
    uint32_t m_packets;
    double m_delay;
    std::list<TcpFileTransfer*> m_fileTransfers;
    double m_linkLatency;
  public:
    void SetPropagationDelay (double latency) 
    {
      m_linkLatency = latency;
    }

    void RetransmitCallback (std::string path, bool fastRetransmit)
    {
      std::cout << path << "," << (fastRetransmit ? "FR" : "T") << std::endl;
    }

    void ReceivePacketCallback (Ptr<const Packet> packet, const Address& addr)
    {
      Ipv4Address ipAddr = InetSocketAddress::ConvertFrom(addr).GetIpv4();
      std::cout << m_addressToNodeMap[ipAddr] << ",RX," << Simulator::Now().ToDouble(Time::US) << "," << packet->GetSize() << std::endl;
    }

    inline uint32_t CannonicalNode (const uint32_t node) 
    {
      return m_nodeTranslate[node];
    }

    inline uint32_t AddressForNode (const Ipv4Address address) 
    {
      return m_addressToNodeMap[address];
    }

    void SetDelay(double delay)
    {
      m_delay = delay;
    }
    void AddPathsToTest(std::vector<std::pair<uint32_t, uint32_t> > paths)
    {
      NS_LOG_LOGIC("Add paths to test");
      m_pathsToTest.insert(m_pathsToTest.end(), paths.begin(), paths.end());
      for (;m_currentPath < m_pathsToTest.size(); m_currentPath++) {
        uint32_t client = m_nodeForwardTranslationMap[m_pathsToTest[m_currentPath].first];
        uint32_t server = m_nodeForwardTranslationMap[m_pathsToTest[m_currentPath].second];
        TcpFileTransfer* fileTransfer = new TcpFileTransfer(m_nodes.Get(client), m_nodes.Get(server));
        m_fileTransfers.push_back(fileTransfer);
        Simulator::Schedule(Seconds(2.0), &TcpFileTransfer::StartSending, fileTransfer);
      }
      NS_LOG_LOGIC("Add paths to test");
    }

    void RouteEnded ()
    {
      NS_ASSERT(false);
      NS_LOG_LOGIC("Packet ended");
      m_currentTrial++;
    }
    
    void SetPackets(uint32_t packets)
    {
      m_packets = packets;
    }

    Topology()
    {
      m_numNodes = 0;
      m_currentPath = 0;
      m_currentTrial = 0;
      m_simulationEnd = Seconds(60.0 * 60.0 * 24 * 7);
      m_packets = 0;
      m_linkLatency = 1.0;
   }
   
    virtual ~Topology()
    {
        for (uint32_t i = 0; i < m_numNodes; i++) {
            delete m_connectivityGraph[i];
        }
    }
    void PopulateGraph(std::string& filename)
    {
      std::map<uint32_t, std::list<uint32_t>*> tempConnectivityGraph;
      NS_LOG_INFO("Entering PopulateGraph with file " << filename);
      std::ifstream topology(filename.c_str());
      NS_ASSERT(topology.is_open());
    
      while (topology.good()) {
        std::string input;
        getline(topology, input);
        size_t found = input.find(" ");
        if (found == std::string::npos) {
          continue;
        }
        std::string node1s = input.substr(0, int(found));
        std::string node2s = input.substr(int(found) + 1, input.length() - (int(found) + 1));
        uint32_t node1 = std::atoi(node1s.c_str()) ;
        uint32_t node2 = std::atoi(node2s.c_str());
        if (!tempConnectivityGraph[node1]) {
          tempConnectivityGraph[node1] = new std::list<uint32_t>;
        }
        if (!tempConnectivityGraph[node2]) {
          tempConnectivityGraph[node2] = new std::list<uint32_t>;
        }
        tempConnectivityGraph[node1]->push_back(node2);
        tempConnectivityGraph[node2]->push_back(node1);
      }
    
      m_numNodes = tempConnectivityGraph.size();
      uint32_t last = 0;
      m_connectivityGraph.resize(m_numNodes);
    
      for (std::map<uint32_t, std::list<uint32_t>*>::iterator it = tempConnectivityGraph.begin();
          it != tempConnectivityGraph.end();
          it++) {
          m_nodeTranslate.push_back(it->first);
          m_nodeForwardTranslationMap[it->first] = last;
          NS_LOG_LOGIC("TRANSLATION: " << it->first << " = " << last);
          last++;
      }
      for (uint32_t i = 0; i < m_numNodes; i++) {
        m_connectivityGraph[i] = new std::list<uint32_t>;
        for (std::list<uint32_t>::iterator it = tempConnectivityGraph[m_nodeTranslate[i]]->begin();
             it != tempConnectivityGraph[m_nodeTranslate[i]]->end();
             it++) {
          m_connectivityGraph[i]->push_back(m_nodeForwardTranslationMap[*it]);
        }
        NS_ASSERT_MSG(!m_connectivityGraph[i]->empty(), "Empty for " << i);
      }
    }

    void HookupSimulation()
    {
      NS_LOG_INFO("Creating nodes");
      m_nodes.Create (m_numNodes);
      m_nodeDevices.resize(m_numNodes);
      NS_LOG_INFO("Creating point to point connections");
      PointToPointHelper pointToPoint;
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
      pointToPoint.SetChannelAttribute("Delay", TimeValue(MilliSeconds(m_linkLatency))); 
      for (uint32_t i = 0; i < m_numNodes; i++) {
        m_callbacks.push_back(NodeCallback(m_nodeTranslate[i], this));
        NS_ASSERT(!m_connectivityGraph[i]->empty());
        for ( std::list<uint32_t>::iterator iterator = m_connectivityGraph[i]->begin(); 
        iterator != m_connectivityGraph[i]->end();
        iterator++) {
          if (*iterator < i) {
            continue;
          }
          pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue(DataRate("10Gbps")));
          pointToPoint.SetChannelAttribute("Delay", TimeValue(MilliSeconds(m_linkLatency))); 
          NetDeviceContainer p2pDevices = 
            pointToPoint.Install (m_nodes.Get(i), m_nodes.Get(*iterator));
          m_nodeDevices[i].Add(p2pDevices.Get(0));
          p2pDevices.Get(0)->TraceConnectWithoutContext("MacTxDrop", MakeCallback(&NodeCallback::PhyDropTrace, &m_callbacks[i]));
          m_nodeDevices[*iterator].Add(p2pDevices.Get(1));
          p2pDevices.Get(1)->TraceConnectWithoutContext("MacTxDrop", MakeCallback(&NodeCallback::PhyDropTrace, &m_callbacks[*iterator]));
          m_linkDevices.push_back(p2pDevices);
          PointToPointChannel* channel = (PointToPointChannel*)GetPointer(p2pDevices.Get(0)->GetChannel());
          m_channels.push_back(channel);
          std::pair<uint32_t, uint32_t> normalizedLink;
          if (m_nodeTranslate[i] < m_nodeTranslate[*iterator]) {
            normalizedLink = std::pair<uint32_t, uint32_t>(m_nodeTranslate[i], m_nodeTranslate[*iterator]);
          }
          else {
            normalizedLink = std::pair<uint32_t, uint32_t>(m_nodeTranslate[i], m_nodeTranslate[*iterator]);
          }
          m_channelMap[normalizedLink] = channel;
        }
      }

      InternetStackHelper stack;
      stack.Install (m_nodes);
      Ipv4AddressHelper address;
      address.SetBase ("10.1.1.0", "255.255.255.0");
      NS_LOG_INFO("Assigning address");
      for (int i = 0; i < (int)m_linkDevices.size(); i++) {
        Ipv4InterfaceContainer current = address.Assign(m_linkDevices[i]);
        address.NewNetwork();
      }
      NS_LOG_INFO("Assigned addresses");

      for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); it++) {
        Ptr<Node> node = *it;
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        for (uint32_t i = 1; i < ipv4->GetNInterfaces (); i++) {
          for (uint32_t j = 0; j < ipv4->GetNAddresses (i); j++) {
            m_addressToNodeMap[ipv4->GetAddress(i, j).GetLocal ()] = node->GetId(); 
          }
        }
      }
      NS_LOG_INFO("Added addresses");

      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

      NS_LOG_INFO("Done populating routing table");

      UdpEchoServerHelper echoServer (9);
      PacketSinkHelper sink ("ns3::TcpSocketFactory", 
                             InetSocketAddress (Ipv4Address::GetAny (), PORT)); 

      ApplicationContainer serverApps = echoServer.Install (m_nodes);
      ApplicationContainer sinks = sink.Install (m_nodes);
      sinks.Start (Seconds (0.0));
      serverApps.Start (Seconds (1.0));
      serverApps.Stop (m_simulationEnd);
      m_clients.resize(m_numNodes);
      m_servers.resize(m_numNodes);
      for (uint32_t i = 0; i < m_numNodes; i++) {
        sinks.Get(i)->TraceConnectWithoutContext(std::string("Rx"), MakeCallback(&Topology::ReceivePacketCallback, this));
        Ptr<GlobalRouter> router = m_nodes.Get(i)->GetObject<GlobalRouter>();
        Ptr<Ipv4GlobalRouting> gr = router->GetRoutingProtocol();
        gr->SetAttribute("ReverseOutputToInputDelay", TimeValue(MilliSeconds(m_delay * 1e-6)));
        gr->SetAttribute("ReverseInputToOutputDelay", TimeValue(MilliSeconds(m_delay * 1e-6)));
        Ptr<Ipv4L3Protocol> l3 = m_nodes.Get(i)->GetObject<Ipv4L3Protocol>();
        l3->TraceConnectWithoutContext("Drop", MakeCallback(&NodeCallback::DropTrace, &m_callbacks[i]));
        l3->SetAttribute("DefaultTtl", UintegerValue(255));
        gr->AddReversalCallback(MakeCallback(&NodeCallback::NodeReversal, &m_callbacks[i]));
        m_servers[i] =  (UdpEchoServer*)PeekPointer(serverApps.Get(i));
        m_servers[i]->AddReceivePacketEvent(MakeCallback(&NodeCallback::ServerRxPacket, &m_callbacks[i]));
      }
      NS_LOG_INFO("Done setting up simulation");
    }

    void Reset ()
    {
      SimulationSingleton<GlobalRouteManagerImpl>::Get ()->SendHeartbeats();
    }

    void PingMachines (UdpEchoClient* client)
    {
      NS_LOG_LOGIC("Untranslated sending between " << client);
      NS_LOG_LOGIC("Sending between " << client);
      Simulator::ScheduleNow(&UdpEchoClient::StartApplication, client);
      uint32_t controlFlag = Ipv4Header::GetControlFlag();
      Simulator::Schedule(Seconds(1.0), &UdpEchoClient::SendBurst, client, m_packets, MilliSeconds(0.012), controlFlag);
    }
    
    void FailLink (uint32_t from, uint32_t to)
    {
      NS_ASSERT(from < to);
      NS_LOG_LOGIC("Failing link between " << from << " and " << to);
      FailLinkPair(std::pair<uint32_t, uint32_t>(from, to));
    }

    void FailLinkPair(std::pair<uint32_t, uint32_t> key)
    {
      NS_LOG_LOGIC("Failing link between " << key.first << " and " << key.second);
      std::cout << key.first << "," << key.second << ",F" << std::endl;
      m_channelMap[key]->SetLinkDown();
    }
};


void NodeCallback::RxPacket (Ptr<const Packet> packet, Ipv4Header& header) {
  NS_LOG_LOGIC(m_id << " Received packet " << (uint32_t)header.GetTtl());
  uint32_t seq = 0;
  packet->CopyData ((uint8_t*)&seq, sizeof(seq));
  std::cout << m_topology->CannonicalNode(m_topology->AddressForNode(header.GetSource()))
            <<"," << m_id << ","<< seq << std::endl;
}

void NodeCallback::ServerRxPacket (Ptr<const Packet> packet, Ipv4Header& header) {
  NS_LOG_LOGIC(m_id << " Server Received packet " << (uint32_t)header.GetTtl());
  uint32_t seq = 0;
  packet->CopyData ((uint8_t*)&seq, sizeof(seq));
  std::cout << m_topology->CannonicalNode(m_topology->AddressForNode(header.GetSource()))
            <<"," << m_id << ","<< seq << std::endl;
  //std::cout << seq << ",";
}

void NodeCallback::NodeReversal (uint32_t iface, Ipv4Address addr) {
  NS_LOG_LOGIC(m_id << " reversed iface " << iface << " for " << addr);
  std::cout << m_id << ",R" << std::endl;
}

void NodeCallback::DropTrace (const Ipv4Header& hdr, Ptr<const Packet> packet, Ipv4L3Protocol::DropReason drop, Ptr<Ipv4> ipv4, uint32_t iface) {
  NS_LOG_LOGIC(m_id << " dropped packet " << iface);
  std::cout << m_topology->CannonicalNode(m_topology->AddressForNode(hdr.GetSource()))
            << "," << m_topology->CannonicalNode(m_topology->AddressForNode(hdr.GetDestination()))
            << ",D(" << (uint32_t)hdr.GetTtl() << ")" << std::endl;
}

void NodeCallback::PhyDropTrace (Ptr<const Packet> packet) {
  std::cout << m_id << ",P" << std::endl;
}

void
ParseLinks(std::string links, std::vector<std::pair<uint32_t, uint32_t> >& results)
{
  std::vector<std::string> linkParts;
  
  boost::split(linkParts, links, boost::is_any_of(","));
  for (std::vector<std::string>::iterator it = linkParts.begin(); 
       it != linkParts.end(); it++) {
    std::vector<std::string> nodeParts;
    boost::split(nodeParts, *it, boost::is_any_of("="));
    NS_ASSERT(nodeParts.size() == 2);
    results.push_back(std::pair<uint32_t, uint32_t>(std::atoi(nodeParts[0].c_str()), std::atoi(nodeParts[1].c_str())));
  }
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiHelper;

  Ptr<OutputStreamWrapper> stream = asciiHelper.CreateFileStream ("tcp-trace.tr");
  bool simulateError;
  std::string topology;
  std::string links;
  std::string paths;
  uint32_t packets = 1;
  double delay = 0.0;
  double linkLatency = 1.0;
  std::vector<std::pair<uint32_t, uint32_t> > linksToFail;
  std::vector<std::pair<uint32_t, uint32_t> > pathsToTest;
  CommandLine cmd;
  cmd.AddValue("packets", "Number of packets to echo", packets);
  cmd.AddValue("error", "Simulate error", simulateError);
  cmd.AddValue("topology", "Topology file", topology);
  cmd.AddValue("links", "Links to fail", links);
  cmd.AddValue("paths", "Source destination pairs to test", paths);
  cmd.AddValue("packets", "Packets to send per trial", packets);
  cmd.AddValue("delay", "Delay for repairs", delay);
  cmd.AddValue("latency", "Propagation delay (ms)", linkLatency);
  cmd.Parse(argc, argv);
  if (!links.empty()) {
    ParseLinks(links, linksToFail);
  }
  if (!paths.empty()) {
    ParseLinks(paths, pathsToTest);
  }
  Topology simulationTopology;
  simulationTopology.SetDelay(delay);
  simulationTopology.SetPackets(packets);
  simulationTopology.SetPropagationDelay(linkLatency);
  simulationTopology.PopulateGraph(topology);
  simulationTopology.HookupSimulation();
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //Simulator::ScheduleNow(&Topology::FailLink, &simulationTopology, 5, 6);
  //Simulator::ScheduleNow(&Topology::FailLink, &simulationTopology, 6, 7);
  WeibullVariable weibull(2.0, 0.5);
  for (std::vector<std::pair<uint32_t, uint32_t> >::iterator it = linksToFail.begin();
       it != linksToFail.end();
       it++) {
    double failTime = 3.0 + weibull.GetValue();
    Simulator::Schedule(Seconds(failTime), &Topology::FailLinkPair, &simulationTopology, *it);
  }
  simulationTopology.AddPathsToTest(pathsToTest);
  //Simulator::Schedule(Seconds(1.0), &Topology::PingMachines, &simulationTopology, 1, 6);
  //simulationTopology.PingMachines(1, 6);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
