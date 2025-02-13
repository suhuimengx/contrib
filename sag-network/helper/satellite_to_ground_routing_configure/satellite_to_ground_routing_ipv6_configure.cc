/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 ETH Zurich
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
 *
 */

#include "satellite_to_ground_routing_ipv6_configure.h"

namespace ns3 {

SatellitetoGroundRoutingIPv6Configure::SatellitetoGroundRoutingIPv6Configure(Ptr<BasicSimulation> basicSimulation, std::vector<std::pair<SAGRoutingHelperIPv6, int16_t>>& sagRoutings)
{

    printf("PROTOCOL CONFIGURATION SATELLITE TO GROUND ROUTING IPV6\n");
    m_basicSimulation = basicSimulation;
    // Check if it is enabled explicitly
    m_enabled = parse_boolean(basicSimulation->GetConfigParamOrDefault("enable_satellite_to_ground_routing_ipv6", "false"));
	if (!m_enabled) {
		std::cout << "  > Not enabled explicitly for satellite to ground routing ipv6, so disabled" << std::endl;
	}
	else{
		int16_t priority = stoi(basicSimulation->GetConfigParamOrFail("priority_of_satellite_to_ground_routing_ipv6"));
		m_routingHelper = SAG_GS_Static_Routing_Helper_IPv6();
		m_routingHelper.SetObjectNameString(basicSimulation->GetConfigParamOrFail("installation_of_satellite_to_ground_routing_ipv6"));
		/// set attributes...
		sagRoutings.push_back(std::make_pair(m_routingHelper, priority));
	}

    std::cout << std::endl;
}

void SatellitetoGroundRoutingIPv6Configure::Initialize(Ptr<TopologySatelliteNetwork> topology){
	if (m_enabled) {
		m_routingHelper.SetTopologyHandle(topology->GetConstellations());
		m_routingHelper.InitializeArbiter(m_basicSimulation, topology->GetNodes());
	}

}




}
