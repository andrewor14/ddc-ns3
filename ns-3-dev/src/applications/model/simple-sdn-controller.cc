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

#include "simple-sdn-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleSDNControllerApplication");

TypeId
SimpleSDNController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleSDNController")
    .SetParent<Application> ()
    .AddConstructor<SimpleSDNController> ()
    .AddAttribute ("ID", "Controller ID.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&SimpleSDNController::m_id),
                   MakeUintegerChecker<uint32_t> ())
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
  m_leader_id = UINT32_MAX;
}

SimpleSDNController::~SimpleSDNController()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_receive_socket = 0;
  m_switch_addresses.clear ();
  m_controller_addresses.clear ();
  m_send_sockets.clear ();
}

void
SimpleSDNController::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
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

  // Set up send sockets
  std::list<InetSocketAddress>::iterator it;
  for (it = m_switch_addresses.begin (); it != m_switch_addresses.end (); it++) {
    CreateSendSocket(*it);
  }
  for (it = m_controller_addresses.begin (); it != m_controller_addresses.end (); it++) {
    CreateSendSocket(*it);
  }

  // Begin pinging peers
  Simulator::Schedule (m_ping_switches_interval, &SimpleSDNController::PingSwitches, this);
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
  if (m_id == m_leader_id) {
    // This controller is the leader
    // Send a packet to all known switches
    std::list<InetSocketAddress>::iterator it;
    for (it = m_switch_addresses.begin ();
         it != m_switch_addresses.end ();
         it++) {
      InetSocketAddress address = *it;
      Ptr<Packet> p = Create<Packet> ();
      // Set up application header
      SimpleSDNHeader sdnHeader;
      sdnHeader.SetControllerID (m_id);
      sdnHeader.SetRespondPort (m_port);
      // Set up IP header
      uint32_t control_flag = Ipv4Header::GetControlFlag ();
      Ipv4Header ipHeader;
      ipHeader.SetFlags (control_flag);
      ipHeader.SetDestination (address.GetIpv4 ());
      // Actually send the packet
      m_txTrace (p, ipHeader);
      Ptr<Socket> socket = m_send_sockets.at (address);
      socket->Send (p, control_flag);
    }
  }
  Simulator::Schedule (m_ping_switches_interval, &SimpleSDNController::PingSwitches, this);
}

void
SimpleSDNController::PingControllers ()
{
  // TO BE IMPLEMENTED
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
      NS_LOG_INFO (
        "At time " << Simulator::Now ().GetSeconds () <<
        "s controller received " << packet->GetSize () << " bytes from " <<
        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
        InetSocketAddress::ConvertFrom (from).GetPort ());

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      // TODO: Add some important logic here

      m_txTrace(packet, hdr);
      socket->SendTo (packet, hdr.GetFlags(), from);

      NS_LOG_INFO (
        "At time " << Simulator::Now ().GetSeconds () <<
        "s controller sent " << packet->GetSize () << " bytes to " <<
        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
        InetSocketAddress::ConvertFrom (from).GetPort ());
    }
  }
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
