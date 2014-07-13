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

#ifndef SIMPLE_SDN_HEADER_H
#define SIMPLE_SDN_HEADER_H

#include "ns3/header.h"

namespace ns3 {
/**
 * @aor
 * \class SimpleSDNHeader
 * \brief Packet header for simple SDN controllers and switches.
 */
class SimpleSDNHeader : public Header
{
public:
  SimpleSDNHeader ();

  /**
   * \param controller_id the controller ID
   */
  void SetControllerID (uint32_t controller_id);

  /**
   * \param leader_id the ID of the leader
   */
  void SetLeaderID (uint32_t leader_id);

  /**
   * \param respond_port the port at which the receiver
   *                     of this packet should respond
   */
  void SetRespondPort (uint16_t respond_port);

  /**
   * \param epoch the current epoch
   */
  void SetEpoch (uint32_t epoch);

  /**
   * \param time_sent the time the packet is sent
   */
  void SetTimeSent (uint64_t time_sent);

  /**
   * \return the controller ID
   */
  uint32_t GetControllerID (void) const;

  /**
   * \return the ID of the current leader
   */
  uint32_t GetLeaderID (void) const;

  /**
   * \return the port at which the receiver at this packet should respond
   */
  uint16_t GetRespondPort (void) const;

  /**
   * \return the current epoch
   */
  uint32_t GetEpoch (void) const;

  /**
   * \return the time at which the packet was sent
   */
  uint64_t GetTimeSent (void) const;

  static TypeId GetTypeId (void);

private:
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  uint32_t m_controller_id;
  uint32_t m_leader_id;
  uint16_t m_respond_port;
  uint32_t m_epoch;
  uint64_t m_time_sent;
};

} // namespace ns3

#endif /* SIMPLE_SDN_HEADER_H */
