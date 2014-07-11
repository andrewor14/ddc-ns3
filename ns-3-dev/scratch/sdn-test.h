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

using namespace ns3;

// Number of nodes in this network
const int32_t numNodes = 12;

// Graph representation of the network
std::list<uint32_t>* connectivityGraph [numNodes];

// Controller attributes
const uint32_t controllerPort = 2244;
const Time controllerPingSwitchesInterval = Time (Seconds (1.0));
const Time controllerPingControllersInterval = Time (Seconds (0.1));
const uint32_t controllerMaxEpoch = 80;

// Switch attributes
const uint32_t switchPort = 3355;
const Time switchWindowDuration = controllerPingSwitchesInterval;
const uint8_t switchMaxViolationCount = 3;

// Simulation end time
Time simulationEnd = Seconds(60.0 * 60.0 * 24 * 7);

