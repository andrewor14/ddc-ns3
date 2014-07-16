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

#ifndef SIMPLE_SDN_SWITCH_H
#define SIMPLE_SDN_SWITCH_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/nstime.h"

#include <list>

namespace ns3 {

class Socket;
class Packet;

/**
 * @aor
 * \brief A simple SDN switch.
 * A switch that accepts controller pings and detects control plane inconsistency.
 * Each switch should belong to only one controller.
 */
class SimpleSDNSwitch : public Application 
{
public:
  static TypeId GetTypeId (void);
  SimpleSDNSwitch ();
  SimpleSDNSwitch (uint32_t);
  virtual ~SimpleSDNSwitch ();
  void SetID (uint32_t);
  void SetPort (uint16_t);
  uint32_t GetID (void) const;
  uint16_t GetPort (void) const;
  void AddReceivePacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent);
  void AddTransmitPacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > txEvent);

  static uint32_t numSwitchesWithViolation;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  /*
   * A simple SDN switch assumes we receive packets from controllers on a window basis.
   * In every window, the switch records the controller IDs from the packets it received.
   * If more than one controller has contacted this switch within the past window, we
   * consider this a potential inconsistency in the control plane. Then, we report
   * violation after the same set of controllers have consistently contacted this switch.
   */
  void UpdateWindow (void);
  void ReportViolation(void);

  // The duration of each window
  Time m_window_duration;

  // Controller IDs that have contacted this switch in this window
  std::list<uint32_t> m_current_controllers;

  // Controller IDs that have contacted this switch in the previous window
  std::list<uint32_t> m_previous_controllers;

  // Number of windows in which multiple controllers (the same set) have contacted this switch
  uint8_t m_violation_count;
  uint8_t m_max_violation_count;

  // StartApplication is being called twice for some reason...
  bool m_application_started;

  uint16_t m_port;
  uint32_t m_id;
  Ptr<Socket> m_socket;

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_rxTrace;
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_txTrace;
};

} // namespace ns3

#endif /* SIMPLE_SDN_SWITCH_H */

