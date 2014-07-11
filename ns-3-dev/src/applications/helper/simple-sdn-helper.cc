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
#include "ns3/ipv4.h"

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
  return ApplicationContainer (InstallPrivate (node, id));
}

ApplicationContainer
SimpleSDNControllerHelper::Install (NodeContainer nodes, uint32_t start_id) const
{
  ApplicationContainer apps;
  NodeContainer::Iterator it;
  for (it = nodes.Begin (); it != nodes.End (); it++) {
    apps.Add (InstallPrivate (*it, start_id++));
  }

  // Add all controllers as peers to each other
  uint32_t numApps = apps.GetN ();
  for (uint32_t i = 0; i < numApps; i++) {
    SimpleSDNController* controller = (SimpleSDNController*) GetPointer (apps.Get (i));
    for (uint32_t j = 0; j < numApps; j++) {
      if (i != j) {
        SimpleSDNController* otherController = (SimpleSDNController*) GetPointer (apps.Get (j));
        UintegerValue portValue;
        otherController->GetAttribute ("Port", portValue);
        uint16_t port = (uint16_t) portValue.Get ();
        Ipv4Address ipv4 = nodes.Get (j)->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
        InetSocketAddress* address = new InetSocketAddress (ipv4, port);
        controller->AddPeeringController (*address);
      }
    }
  }
  return apps;
}

void
SimpleSDNControllerHelper::ConnectToSwitches (NodeContainer controllers, NodeContainer switches)
{
  // Add all switches to all controllers as ppers
  NodeContainer::Iterator it_c;
  NodeContainer::Iterator it_s;
  for (it_c = controllers.Begin (); it_c != controllers.End (); it_c++) {
    Node* node_c = (Node*) GetPointer (*it_c);
    SimpleSDNController* app_c = (SimpleSDNController*) GetPointer (node_c->GetApplication (0));
    for (it_s = switches.Begin (); it_s != switches.End (); it_s++) {
      Node* node_s = (Node*) GetPointer (*it_s);
      SimpleSDNSwitch* app_s = (SimpleSDNSwitch*) GetPointer (node_s->GetApplication (0));
      UintegerValue portValue;
      app_s->GetAttribute ("Port", portValue);
      uint16_t port = (uint16_t) portValue.Get ();
      Ipv4Address ipv4 = node_s->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
      InetSocketAddress* switchAddress = new InetSocketAddress (ipv4, port);
      app_c->AddPeeringSwitch (*switchAddress);
    }
  }
}


Ptr<Application>
SimpleSDNControllerHelper::InstallPrivate (Ptr<Node> node, uint32_t id) const
{
  Ptr<Application> app = m_factory.Create<SimpleSDNController> ();
  SimpleSDNController* sdnController = (SimpleSDNController*) GetPointer (app);
  sdnController->SetID (id);
  sdnController->SetLeaderID (id);
  node->AddApplication (app);
  return app;
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
  return ApplicationContainer (InstallPrivate (node, id));
}

ApplicationContainer
SimpleSDNSwitchHelper::Install (NodeContainer nodes, uint32_t start_id) const
{
  ApplicationContainer appContainer;
  for (NodeContainer::Iterator it = nodes.Begin (); it != nodes.End (); it++) {
    appContainer.Add (InstallPrivate (*it, start_id++));
  }
  return appContainer;
}

Ptr<Application>
SimpleSDNSwitchHelper::InstallPrivate (Ptr<Node> node, uint32_t id) const
{
  Ptr<Application> app = m_factory.Create<SimpleSDNSwitch> ();
  SimpleSDNSwitch* sdnSwitch = (SimpleSDNSwitch*) GetPointer (app);
  sdnSwitch->SetID (id);
  node->AddApplication (app);
  return app;
}

} // namespace ns3
