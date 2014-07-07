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
#include "ns3/traced-callback.h"
#include "ns3/ipv4-header.h"

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
  virtual ~SimpleSDNController ();
  void AddReceivePacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > rxEvent);
  void AddTransmitPacketEvent (Callback<void, Ptr<const Packet>, Ipv4Header& > txEvent);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  uint16_t m_port;
  uint32_t m_id;
  Ptr<Socket> m_socket;
  Address m_local;
  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_rxTrace;
  TracedCallback<Ptr<const Packet>, Ipv4Header&> m_txTrace;
};

} // namespace ns3

#endif /* SIMPLE_SDN_CONTROLLER_H */

