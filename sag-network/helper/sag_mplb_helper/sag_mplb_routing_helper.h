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

#ifndef SAG_MPLB_ROUTING_HELPER_H
#define SAG_MPLB_ROUTING_HELPER_H

#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_helper.h"

namespace ns3 {

class Sag_MPLB_Routing_Helper: public SAGRoutingHelper {
public:
	static TypeId GetTypeId(void);
	Sag_MPLB_Routing_Helper();
	Sag_MPLB_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

};

} // namespace ns3

#endif /* SAG_MPLB_ROUTING_HELPER_H */
