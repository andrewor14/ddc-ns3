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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "simple-sdn-header.h"

NS_LOG_COMPONENT_DEFINE ("SimpleSDNHeader");

namespace ns3 {

TypeId
SimpleSDNHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleSDNHeader")
    .SetParent<Header> ()
    .AddConstructor<SimpleSDNHeader> ();
  return tid;
}

TypeId
SimpleSDNHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

SimpleSDNHeader::SimpleSDNHeader ():
  m_controller_id (0),
  m_respond_port (0) { }

void
SimpleSDNHeader::SetControllerID (uint32_t controller_id)
{
  m_controller_id = controller_id;
}

uint32_t
SimpleSDNHeader::GetControllerID (void) const
{
  return m_controller_id;
}

void
SimpleSDNHeader::SetRespondPort (uint16_t respond_port)
{
  m_respond_port = respond_port;
}

uint16_t
SimpleSDNHeader::GetRespondPort (void) const
{
  return m_respond_port;
}

void
SimpleSDNHeader::Print (std::ostream &os) const
{
  os << "(" <<
    "controller_id=" << m_controller_id << " " <<
    "respond_port=" << m_respond_port << ")";
}

uint32_t
SimpleSDNHeader::GetSerializedSize (void) const
{
  return 4+2;
}

void
SimpleSDNHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_controller_id);
  i.WriteHtonU16 (m_respond_port);
}

uint32_t
SimpleSDNHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_controller_id = i.ReadNtohU32 ();
  m_respond_port = i.ReadNtohU16 ();
  return GetSerializedSize ();
}

} // namespace ns3
