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

#ifndef SIMPLE_SDN_HELPER_H
#define SIMPLE_SDN_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <stdint.h>

namespace ns3 {

/**
 * \brief Create a controller that periodically pings its peering switches and controllers.
 *        A group of controllers elect a leader among themselves through pings.
 */
class SimpleSDNControllerHelper
{
public:
  /**
   * Create a SimpleSDNControllerHelper.
   */
  SimpleSDNControllerHelper ();

  /**
   * Create a SimpleSDNControllerHelper.
   *
   * \param port The port on which the controller listens
   * \param ping_switches_interval Interval between which the controller pings its switches
   * \param ping_controllers_interval Interval between which the controller pings other controllers
   */
  SimpleSDNControllerHelper (
    uint16_t port,
    Time ping_switches_interval,
    Time ping_controllers_interval,
    uint32_t max_epoch);

  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Install SimpleSDNController on the specified node(s).
   * If multiple nodes are provided, automatically connect all nodes as peers.
   */
  ApplicationContainer Install (Ptr<Node> node, uint32_t id) const;
  ApplicationContainer Install (NodeContainer nodes, uint32_t start_id) const;

  /**
   * Connect all controllers to all switches.
   */
  void ConnectToSwitches (NodeContainer controllers, NodeContainer switches);

private:
  ObjectFactory m_factory;
  Ptr<Application> InstallPrivate (Ptr<Node> node, uint32_t id) const;
};

/**
 * \brief Create a switch that listens for controller pings.
 */
class SimpleSDNSwitchHelper
{
public:
  /**
   * Create a SimpleSDNSwitchHelper.
   */
  SimpleSDNSwitchHelper ();

  /**
   * Create a SimpleSDNSwitchHelper.
   *
   * \param port The port on which the switch listens
   * \param windowDuration duration of each window to receive packets from controllers
   * \param maxViolationCount maximum number of windows with control plane inconsistency
   */
  SimpleSDNSwitchHelper (
    uint16_t port,
    Time windowDuration,
    uint8_t maxViolationCount);

  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Install SimpleSDNSwitch on the specified node(s).
   */
  ApplicationContainer Install (Ptr<Node> node, uint32_t id) const;
  ApplicationContainer Install (NodeContainer nodes, uint32_t start_id) const;

private:
  ObjectFactory m_factory;
  Ptr<Application> InstallPrivate (Ptr<Node> node, uint32_t id) const;
};

} // namespace ns3

#endif /* SIMPLE_SDN_HELPER_H */
