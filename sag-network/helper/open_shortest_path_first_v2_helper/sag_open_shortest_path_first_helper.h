/*
 * Copyright (c) 2023 NJU
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
 * Author: Xiaoyu Liu <xyliu0119@163.com>
 */

#ifndef SAG_OPEN_SHORTEST_PATH_FIRST_HELPER_H
#define SAG_OPEN_SHORTEST_PATH_FIRST_HELPER_H

#include "ns3/basic-simulation.h"
//#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_helper.h"

namespace ns3 {

class Sag_Open_Shortest_Path_First_Helper: public SAGRoutingHelper {
public:
	static TypeId GetTypeId(void);
	Sag_Open_Shortest_Path_First_Helper();
	Sag_Open_Shortest_Path_First_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);
	Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
	void InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

private:
	void UpdateForwardingState(int32_t t);

	// area length and width
	uint32_t m_length;
	uint32_t m_width;
	// areaId
	std::unordered_map<uint32_t, uint32_t> m_areaId;

};

} // namespace ns3

#endif /* SAG_OPEN_SHORTEST_PATH_FIRST_HELPER_H */
