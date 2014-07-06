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

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/packet-metadata.h"
#include "ns3/ipv4-header.h"
#include "control-priority-queue.h"

NS_LOG_COMPONENT_DEFINE ("ControlPriorityQueue");

/*
 * Note that this file is essentially a copy of network/util/drop-tail-queue.cc.
 * Because of linkage issues, this file cannot reside in the network module.
 */

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ControlPriorityQueue);

TypeId ControlPriorityQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::ControlPriorityQueue")
    .SetParent<Queue> ()
    .AddConstructor<ControlPriorityQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&ControlPriorityQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this ControlPriorityQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ControlPriorityQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this ControlPriorityQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&ControlPriorityQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

ControlPriorityQueue::ControlPriorityQueue () :
  Queue (),
  m_data_packets (),
  m_control_packets (),
  m_bytesInQueue (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

ControlPriorityQueue::~ControlPriorityQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
ControlPriorityQueue::SetMode (ControlPriorityQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

ControlPriorityQueue::QueueMode
ControlPriorityQueue::GetMode (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mode;
}

bool 
ControlPriorityQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  // Prioritize control packets by putting them in a different queue
  Ptr<Packet> copy = p->Copy();
  std::cout << "--> Enqueing packet " << p << "\n";
  PacketMetadata::ItemIterator it = copy->BeginItem();
  PacketMetadata::Item item;
  while (it.HasNext()) {
    item = it.Next();
    std::cout << "--> Header is " << item.tid.GetName() << "\n";
  }
  //bool control = header.IsControl();
  //std::cout << "Is control? HEY. " << control << "\n";

  if (m_mode == QUEUE_MODE_PACKETS && (m_data_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  m_bytesInQueue += p->GetSize ();
  m_data_packets.push (p);

  NS_LOG_LOGIC ("Number packets " << m_data_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
ControlPriorityQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_data_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_data_packets.front ();
  m_data_packets.pop ();
  m_bytesInQueue -= p->GetSize ();

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_data_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
ControlPriorityQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_data_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_data_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_data_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

} // namespace ns3

