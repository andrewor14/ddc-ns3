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

#include <stdlib.h>

#include "simple-sdn-switch.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleSDNSwitchApplication");

uint32_t SimpleSDNSwitch::numSwitchesWithViolation = 0;

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

SimpleSDNSwitch::SimpleSDNSwitch (uint32_t id)
{
  SetID (id);
}

SimpleSDNSwitch::~SimpleSDNSwitch()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  m_current_controllers.clear ();
  m_previous_controllers.clear ();
}

void
SimpleSDNSwitch::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
SimpleSDNSwitch::SetID (uint32_t id)
{
  m_id = id;
}

void
SimpleSDNSwitch::SetPort (uint16_t port)
{
  m_port = port;
}

uint32_t
SimpleSDNSwitch::GetID (void) const
{
  return m_id;
}

uint16_t
SimpleSDNSwitch::GetPort (void) const
{
  return m_port;
}

void 
SimpleSDNSwitch::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Initialize only once
  if (!m_application_started) {
    m_application_started = true;
    if (m_socket == 0) {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
    }
    m_socket->SetRecvCallback (MakeCallback (&SimpleSDNSwitch::HandleRead, this));
    Simulator::Schedule (m_window_duration, &SimpleSDNSwitch::UpdateWindow, this);
  }
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
      InetSocketAddress returnAddress = InetSocketAddress::ConvertFrom (from);
      NS_LOG_LOGIC (
        "At time " << Simulator::Now ().GetSeconds () << " s " <<
        "switch " << m_id << " " <<
        "received " << packet->GetSize () << " bytes " <<
        "from " << returnAddress.GetIpv4 () << " " <<
        "port " << returnAddress.GetPort ());

      // Respond on the port specified in the SDN header
      SimpleSDNHeader appHeader;
      packet->RemoveHeader (appHeader);
      uint32_t controller_id = appHeader.GetControllerID ();

      // Keep track of the controller ID
      m_current_controllers.push_back (controller_id);
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
  uint8_t previous_violation_count = m_violation_count;

  m_current_controllers.unique ();
  if (m_current_controllers.size() > 1) {
    // Multiple controllers are contacting this switch
    // This is potentially a consistency violation in the control plane

    // Check if the current set of controllers is a subset of the previous set
    std::list<uint32_t> current_controllers_copy = m_current_controllers;
    std::list<uint32_t> previous_controllers_copy = m_previous_controllers;
    previous_controllers_copy.merge (current_controllers_copy);
    previous_controllers_copy.sort ();
    previous_controllers_copy.unique ();
    m_previous_controllers.sort ();
    m_previous_controllers.unique ();

    if (previous_controllers_copy == m_previous_controllers) {
      m_violation_count++;
    } else {
      m_violation_count = 1;
    }
  } else {
    m_violation_count = 0;
  }

  if (previous_violation_count == 0 && m_violation_count > 0) {
    numSwitchesWithViolation++;
  } else if (previous_violation_count > 0 && m_violation_count == 0) {
    numSwitchesWithViolation--;
  }

  if (m_violation_count > 0) {
    NS_LOG_INFO (
      "Switch " << m_id << " " <<
      "has observed " << (uint32_t) m_violation_count << " " <<
      "consecutive violation(s)");
  }

  // If we have exceeded our threshold for number of windows with violation
  if (m_violation_count >= m_max_violation_count) {
    ReportViolation ();
  }

  // Refresh controller IDs
  m_previous_controllers = m_current_controllers;
  m_current_controllers.clear ();

  // Periodically update window
  Simulator::Schedule (m_window_duration, &SimpleSDNSwitch::UpdateWindow, this);
}

void
SimpleSDNSwitch::ReportViolation ()
{
  std::cerr << "Switch " << m_id << " has detected inconsistency in the control plane!";
  exit (EXIT_FAILURE);
}

} // Namespace ns3
