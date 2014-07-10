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

#include "ns3/simple-sdn-controller.h"
#include "ns3/simple-sdn-switch.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

#include "simple-sdn-helper.h"

namespace ns3 {

SimpleSDNControllerHelper::SimpleSDNControllerHelper ()
{
  m_factory.SetTypeId (SimpleSDNController::GetTypeId ());
}

SimpleSDNControllerHelper::SimpleSDNControllerHelper (
  uint16_t port,
  Time ping_switches_interval,
  Time ping_controllers_interval,
  uint32_t max_epoch)
{
  m_factory.SetTypeId (SimpleSDNController::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
  SetAttribute ("PingSwitchesInterval", TimeValue (ping_switches_interval));
  SetAttribute ("PingControllersInterval", TimeValue (ping_controllers_interval));
  SetAttribute ("MaxEpoch", UintegerValue (max_epoch));
}

void 
SimpleSDNControllerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SimpleSDNControllerHelper::Install (Ptr<Node> node, uint32_t id) const
{
  Ptr<Application> app = m_factory.Create<SimpleSDNController> ();
  SimpleSDNController* sdnController = (SimpleSDNController*) GetPointer (app);
  sdnController->SetID (id);
  sdnController->SetLeaderID (id);
  node->AddApplication (app);
  return ApplicationContainer (app);
}

SimpleSDNSwitchHelper::SimpleSDNSwitchHelper ()
{
  m_factory.SetTypeId (SimpleSDNSwitch::GetTypeId ());
}

SimpleSDNSwitchHelper::SimpleSDNSwitchHelper (
  uint16_t port,
  Time windowDuration,
  uint8_t maxViolationCount)
{
  m_factory.SetTypeId (SimpleSDNSwitch::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
  SetAttribute ("WindowDuration", TimeValue (windowDuration));
  SetAttribute ("MaxViolationCount", UintegerValue (maxViolationCount));
}

void 
SimpleSDNSwitchHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SimpleSDNSwitchHelper::Install (Ptr<Node> node, uint32_t id) const
{
  Ptr<Application> app = m_factory.Create<SimpleSDNSwitch> ();
  SimpleSDNSwitch* sdnSwitch = (SimpleSDNSwitch*) GetPointer (app);
  sdnSwitch->SetID (id);
  node->AddApplication (app);
  return ApplicationContainer (app);
}

} // namespace ns3
