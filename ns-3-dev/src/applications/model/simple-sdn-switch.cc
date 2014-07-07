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

#include "simple-sdn-switch.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleSDNSwitchApplication");

TypeId
SimpleSDNSwitch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleSDNSwitch")
    .SetParent<Application> ()
    .AddConstructor<SimpleSDNSwitch> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&SimpleSDNSwitch::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

SimpleSDNSwitch::SimpleSDNSwitch ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

SimpleSDNSwitch::~SimpleSDNSwitch()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

void
SimpleSDNSwitch::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void 
SimpleSDNSwitch::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_socket == 0) {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    m_socket->Bind (local);
  }
  m_socket->SetRecvCallback (MakeCallback (&SimpleSDNSwitch::HandleRead, this));
}

void 
SimpleSDNSwitch::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_socket != 0) {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void 
SimpleSDNSwitch::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  Ipv4Header hdr;
  while ((packet = socket->RecvFrom (from, hdr))) {
    m_rxTrace (packet, hdr);
    if (InetSocketAddress::IsMatchingType (from)) {
      NS_LOG_INFO (
        "At time " << Simulator::Now ().GetSeconds () <<
        "s switch received " << packet->GetSize () << " bytes from " <<
        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
        InetSocketAddress::ConvertFrom (from).GetPort ());
    }
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();

    NS_LOG_LOGIC ("Echoing packet");
    m_txTrace(packet, hdr);
    // @aor: Pass original flags to response packet
    socket->SendTo (packet, hdr.GetFlags(), from);

    if (InetSocketAddress::IsMatchingType (from)) {
      NS_LOG_INFO (
        "At time " << Simulator::Now ().GetSeconds () <<
        "s switch sent " << packet->GetSize () << " bytes to " <<
        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
        InetSocketAddress::ConvertFrom (from).GetPort ());
    }
  }
}

void 
SimpleSDNSwitch::AddReceivePacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent)
{
  m_rxTrace.ConnectWithoutContext(rxEvent);
}

void 
SimpleSDNSwitch::AddTransmitPacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent)
{
  m_txTrace.ConnectWithoutContext(rxEvent);
}
} // Namespace ns3
