/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
 *
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

#ifndef CONTROLPQ_H
#define CONTROLPQ_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that prioritizes control packets over
 * data packets. It is also a drop-tail queue.
 *
 * This file is essentially a copy of network/util/drop-tail-queue.h.
 * Because of linkage issues, this queue cannot reside in the network
 * module, as it references classes in the internet module (Ipv4Header).
 */
class ControlPriorityQueue : public Queue {
public:
  static TypeId GetTypeId (void);
  /**
   * \brief ControlPriorityQueue Constructor
   *
   * Creates a droptail priority queue with a maximum size of 100 packets by default
   */
  ControlPriorityQueue ();

  virtual ~ControlPriorityQueue();

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (ControlPriorityQueue::QueueMode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  ControlPriorityQueue::QueueMode GetMode (void);

private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  std::queue<Ptr<Packet> > m_data_packets;
  std::queue<Ptr<Packet> > m_control_packets;
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  QueueMode m_mode;
};

} // namespace ns3

#endif /* CONTROLPQ_H */
