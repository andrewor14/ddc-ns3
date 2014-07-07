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
                   UintegerValue (8866),
                   MakeUintegerAccessor (&SimpleSDNSwitch::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("WindowDuration", "Duration of each window to receive packets from controllers.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SimpleSDNSwitch::m_window_duration),
                   MakeTimeChecker ())
    .AddAttribute ("MaxViolationCount",
                   "Number of windows of multiple controllers contacting this switch before reporting violation.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&SimpleSDNSwitch::m_max_violation_count),
                   MakeUintegerChecker<uint8_t> ());
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
  Simulator::Schedule (m_window_duration, &SimpleSDNSwitch::UpdateWindow, this);
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

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      // Respond on the port specified in the SDN header
      SimpleSDNHeader appHeader;
      packet->RemoveHeader (appHeader);
      uint16_t respond_port = appHeader.GetRespondPort ();
      uint32_t controller_id = appHeader.GetControllerID ();
      InetSocketAddress returnAddress = InetSocketAddress::ConvertFrom (from);
      returnAddress.SetPort (respond_port);
      appHeader.SetRespondPort (m_port);
      packet->AddHeader (appHeader);
      m_txTrace (packet, hdr);
      socket->SendTo (packet, hdr.GetFlags(), returnAddress);

      // Keep track of the controller ID
      m_current_controllers.push_back (controller_id);

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

void
SimpleSDNSwitch::UpdateWindow ()
{
  if (m_current_controllers.size() > 1) {
    // Multiple controllers are contacting this switch
    // This is potentially a consistency violation in the control plane
    m_current_controllers.sort();
    m_previous_controllers.sort();
    if (m_current_controllers == m_previous_controllers) {
      // Same set of controllers as last window
      m_violation_count++;
    } else {
      // Otherwise, refresh violation count
      m_violation_count = 1;
    }
    if (m_violation_count >= m_max_violation_count) {
      ReportViolation();
    }
  } else {
    m_violation_count = 0;
  }

  // Refresh controller IDs
  m_previous_controllers = m_current_controllers;
  m_current_controllers.clear();

  // Periodically update window
  Simulator::Schedule (m_window_duration, &SimpleSDNSwitch::UpdateWindow, this);
}

void
SimpleSDNSwitch::ReportViolation ()
{
  NS_ABORT_MSG ("Inconsistency in the control plane detected!");
}

} // Namespace ns3
