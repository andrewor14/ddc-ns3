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

#ifndef SIMPLE_SDN_CONTROLLER_H
#define SIMPLE_SDN_CONTROLLER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-header.h"

#include <map>

namespace ns3 {

class Socket;
class Packet;

/**
 * @aor
 * \brief A simple SDN controller.
 * A controller in a control plane distributed using the master/backup scheme.
 * Each group of simple controllers elects a leader based on its controller IDs.
 */
class SimpleSDNController : public Application 
{
public:
  static TypeId GetTypeId (void);
  SimpleSDNController ();
  SimpleSDNController (uint32_t);

  /**
   * Add peering nodes. These must be called before StartApplication.
   */
  void AddPeeringController (InetSocketAddress controllerAddress);
  void AddPeeringSwitch (InetSocketAddress switchAddress);

  void SetID (uint32_t);
  void SetLeaderID (uint32_t);
  void SetPort (uint16_t);
  uint32_t GetID (void) const;
  uint32_t GetLeaderID (void) const;
  uint16_t GetPort (void) const;

  virtual ~SimpleSDNController ();
  void AddReceivePacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent);
  void AddTransmitPacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > txEvent);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
  void HandleControllerRead (Ptr<Packet> p);

  /**
   * Helper method for creating and binding to a send socket.
   */
  void CreateSendSocket (InetSocketAddress address);

  /**
   * Send a special SDN packet to the given address.
   */
  void SendSDNPacket (InetSocketAddress address);

  /**
   * Select a leader based on packets received from other controllers.
   */
  void SelectLeader (void);

  /**
   * If this controller is the leader, send a special SDN packet to all switches.
   * This method calls itself repeatedly after a configurable time interval.
   */
  void PingSwitches (void);

  /**
   * Send a special SDN packet to all peering controllers.
   * Each call to this method is considered the beginning of a new epoch.
   * This method calls itself repeatedly after a configurable time interval.
   */
  void PingControllers (void);

  /**
   * Addresses for peering controllers and switches.
   */
  std::list<InetSocketAddress> m_controller_addresses;
  std::list<InetSocketAddress> m_switch_addresses;
  std::map<InetSocketAddress, Ptr<Socket> > m_send_sockets;

  // Intervals between which this controller pings its peers
  Time m_ping_switches_interval;
  Time m_ping_controllers_interval;

  // A list of controller IDs that represent leader candidates
  std::list<uint32_t> m_leader_candidates;

  // Buffered packets with higher epochs
  std::list<Ptr<Packet> > m_buffered_packets;

  // StartApplication is being called twice for some reason...
  bool m_application_started;

  uint16_t m_port;
  uint32_t m_id;
  uint32_t m_leader_id;
  uint32_t m_epoch;

  // Number of epochs before stopping the simulation
  uint32_t m_max_epoch;

  Ptr<Socket> m_receive_socket;

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_rxTrace;
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_txTrace;
};

} // namespace ns3

#endif /* SIMPLE_SDN_CONTROLLER_H */

