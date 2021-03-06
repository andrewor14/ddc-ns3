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

#include "ns3/random-variable.h"

using namespace ns3;

// Number of nodes in this network
uint32_t numNodes = -1;

// Graph representation of the network
std::vector<std::list<uint32_t>*> connectivityGraph;

// Mapping from old ID to new ID, in case the node ID space is not continuous
std::map<uint32_t, uint32_t> nodeTranslateMap;

// Keep track of all links to fail them
std::vector<PointToPointChannel*> links;
std::vector<PointToPointChannel*> failedLinks;

// Controller attributes
const uint32_t controllerPort = 2244;
const Time controllerPingSwitchesInterval = Seconds (10);
const Time controllerPingControllersInterval = Seconds (1);
uint32_t controllerMaxEpoch = 1000;

// For failing links
UniformVariable rv;
uint32_t numLinksToFail;
Time meanRecoveryDelay;
Time meanFailureDelay;

// Switch attributes
const uint32_t switchPort = 3355;
const Time switchWindowDuration = controllerPingSwitchesInterval;
uint8_t switchMaxViolationCount = 10;

// Interval at which we refresh DDC periodically
uint32_t ddcRefreshInterval = 30; // seconds

