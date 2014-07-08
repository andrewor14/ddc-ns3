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

#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simple-sdn-header.h"

#include <algorithm>

#include "simple-sdn-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleSDNControllerApplication");

TypeId
SimpleSDNController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleSDNController")
    .SetParent<Application> ()
    .AddConstructor<SimpleSDNController> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9966),
                   MakeUintegerAccessor (&SimpleSDNController::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PingSwitchesInterval",
                   "Interval between which this controller pings its peering switches.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SimpleSDNController::m_ping_switches_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PingControllersInterval",
                   "Interval between which this controller pings its peering controllers.",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&SimpleSDNController::m_ping_controllers_interval),
                   MakeTimeChecker ());
  return tid;
}

SimpleSDNController::SimpleSDNController ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ABORT_MSG ("Constructor not supported.");
}

SimpleSDNController::SimpleSDNController (uint32_t id)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_id = id;
  m_leader_id = m_id;
  m_epoch = 0;
}

SimpleSDNController::~SimpleSDNController()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_receive_socket = 0;
  m_switch_addresses.clear ();
  m_controller_addresses.clear ();
  m_send_sockets.clear ();
  m_leader_candidates.clear ();
  m_buffered_packets.clear ();
}

void
SimpleSDNController::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
SimpleSDNController::AddPeeringController (InetSocketAddress controllerAddress)
{
  m_controller_addresses.push_back (controllerAddress);
}

void
SimpleSDNController::AddPeeringSwitch (InetSocketAddress switchAddress)
{
  m_switch_addresses.push_back (switchAddress);
}

void 
SimpleSDNController::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Set up receive socket
  if (m_receive_socket == 0) {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_receive_socket = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    m_receive_socket->Bind (local);
  }
  m_receive_socket->SetRecvCallback (MakeCallback (&SimpleSDNController::HandleRead, this));

  // Set up send sockets. Peers must be set up by the time this is called.
  std::list<InetSocketAddress>::iterator it;
  for (it = m_switch_addresses.begin (); it != m_switch_addresses.end (); it++) {
    CreateSendSocket(*it);
  }
  for (it = m_controller_addresses.begin (); it != m_controller_addresses.end (); it++) {
    CreateSendSocket(*it);
  }

  // Begin pinging peers
  Simulator::Schedule (m_ping_switches_interval, &SimpleSDNController::PingSwitches, this);
  Simulator::Schedule (m_ping_controllers_interval, &SimpleSDNController::PingControllers, this);
}

void 
SimpleSDNController::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_receive_socket != 0) {
    m_receive_socket->Close ();
    m_receive_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
  std::map<InetSocketAddress, Ptr<Socket> >::iterator it;
  for (it = m_send_sockets.begin (); it != m_send_sockets.end (); it++) {
    Ptr<Socket> socket = it->second;
    socket->Close ();
    socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void
SimpleSDNController::CreateSendSocket (InetSocketAddress address)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> socket = Socket::CreateSocket (GetNode (), tid);
  socket->Bind ();
  socket->Connect (address);
  m_send_sockets.at (address) = socket;
}

void
SimpleSDNController::PingSwitches ()
{
  // If I am the leader, ping all switches
  if (m_id == m_leader_id) {
    std::list<InetSocketAddress>::iterator it;
    for (it = m_switch_addresses.begin ();
         it != m_switch_addresses.end ();
         it++) {
      SendSDNPacket(*it);
    }
  }
  Simulator::Schedule (m_ping_switches_interval, &SimpleSDNController::PingSwitches, this);
}

void
SimpleSDNController::PingControllers ()
{
  SelectLeader();
  // This is a new epoch
  m_epoch++;
  std::list<InetSocketAddress>::iterator it;
  for (it = m_controller_addresses.begin ();
       it != m_controller_addresses.end ();
       it++) {
    SendSDNPacket(*it);
  }
  Simulator::Schedule (m_ping_controllers_interval, &SimpleSDNController::PingControllers, this);
}

void
SimpleSDNController::SendSDNPacket (InetSocketAddress address)
{
  Ptr<Packet> p = Create<Packet> ();
  // Set up application header
  SimpleSDNHeader sdnHeader;
  sdnHeader.SetControllerID (m_id);
  sdnHeader.SetLeaderID (m_leader_id);
  sdnHeader.SetRespondPort (m_port);
  sdnHeader.SetEpoch (m_epoch);
  // Set up IP header
  uint32_t control_flag = Ipv4Header::GetControlFlag ();
  Ipv4Header ipHeader;
  ipHeader.SetFlags (control_flag);
  ipHeader.SetDestination (address.GetIpv4 ());
  // Actually send the packet
  m_txTrace (p, ipHeader);
  Ptr<Socket> socket = m_send_sockets.at (address);
  socket->Send (p, control_flag);
  NS_LOG_INFO (
    "At time " << Simulator::Now ().GetSeconds () << "s " <<
    "controller " << m_id << " " <<
    "(leader " << m_leader_id << ") " <<
    "sent "<< p->GetSize () << " bytes " <<
    "to " << address.GetIpv4 () << " " <<
    "port " << address.GetPort () << " " <<
    "(epoch " << m_epoch << ")");
}

void 
SimpleSDNController::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  Ipv4Header hdr;
  while ((packet = socket->RecvFrom (from, hdr))) {
    m_rxTrace (packet, hdr);
    if (InetSocketAddress::IsMatchingType (from)) {
      InetSocketAddress fromAddress = InetSocketAddress::ConvertFrom (from);
      NS_LOG_INFO (
        "At time " << Simulator::Now ().GetSeconds () << "s " <<
        "controller " << m_id << " " <<
        "(leader " << m_leader_id << ") " <<
        "received "<< packet->GetSize () << " bytes " <<
        "from " << fromAddress.GetIpv4 () << " " <<
        "port " << fromAddress.GetPort () << " " <<
        "(epoch " << m_epoch << ")");

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      // Handle only packets received from other controllers
      std::list<InetSocketAddress>::iterator it;
      for (it = m_controller_addresses.begin ();
           it != m_controller_addresses.end ();
           it++) {
        if (fromAddress.GetIpv4 () == it->GetIpv4 ()) {
          HandleControllerRead(packet);
          break;
        }
      }
    }
  }
}

void
SimpleSDNController::HandleControllerRead (Ptr<Packet> p)
{
  SimpleSDNHeader sdnHeader;
  uint32_t controller_id = sdnHeader.GetControllerID ();
  uint32_t leader_id = sdnHeader.GetLeaderID ();
  uint32_t epoch = sdnHeader.GetEpoch ();
  NS_LOG_INFO (
    "  -> Packet received from " <<
    "controller " << controller_id << " " <<
    "(leader " << leader_id << ") has" <<
    "epoch = " << epoch << ". " <<
    "(m_epoch = " << m_epoch << ")");
  if (epoch == m_epoch) {
    uint32_t candidate_id = std::max (controller_id, leader_id);
    m_leader_candidates.push_back (candidate_id);
  } else if (epoch > m_epoch) {
    m_buffered_packets.push_back (p);
  }
}

void
SimpleSDNController::SelectLeader ()
{
  // Collect packets belonging to this epoch from buffer
  std::list<Ptr<Packet> >::iterator it = m_buffered_packets.begin ();
  while (it != m_buffered_packets.end ()) {
    Ptr<Packet> p = *it;
    SimpleSDNHeader sdnHeader;
    uint32_t epoch = sdnHeader.GetEpoch ();
    if (epoch == m_epoch) {
      uint32_t controller_id = sdnHeader.GetControllerID ();
      uint32_t leader_id = sdnHeader.GetLeaderID ();
      uint32_t candidate_id = std::max (controller_id, leader_id);
      m_leader_candidates.push_back (candidate_id);
      m_buffered_packets.erase (it);
    } else {
      it++;
    }
  }

  // Select leader to be the controller with the max ID
  m_leader_candidates.push_back (m_id);
  m_leader_candidates.unique ();
  m_leader_id = *(std::max_element (m_leader_candidates.begin (), m_leader_candidates.end ()));
  m_leader_candidates.clear ();
}

void 
SimpleSDNController::AddReceivePacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent)
{
  m_rxTrace.ConnectWithoutContext(rxEvent);
}

void 
SimpleSDNController::AddTransmitPacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent)
{
  m_txTrace.ConnectWithoutContext(rxEvent);
}
} // Namespace ns3
