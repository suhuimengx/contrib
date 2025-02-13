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
#ifndef OPEN_SHORTEST_PATH_FIRST_CONFIGURE
#define OPEN_SHORTEST_PATH_FIRST_CONFIGURE

#include "ns3/basic-simulation.h"
#include "ns3/sag_routing_helper.h"
#include "ns3/sag_open_shortest_path_first_helper.h"
#include "ns3/topology-satellite-network.h"

namespace ns3 {

class OpenShortestPathFirstConfigure
{

public:
	OpenShortestPathFirstConfigure(Ptr<BasicSimulation> basicSimulation, std::vector<std::pair<SAGRoutingHelper, int16_t>>& sagRoutings);

	void Initialize(Ptr<TopologySatelliteNetwork> topology);

private:
	bool m_enabled;
	Sag_Open_Shortest_Path_First_Helper m_routingHelper;
	Ptr<BasicSimulation> m_basicSimulation;

};

}

#endif /* OPEN_SHORTEST_PATH_FIRST_CONFIGURE */
