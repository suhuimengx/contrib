/*
 * sag_fybbr_routing_helper.h
 *
 *  Created on: 2023年1月31日
 *      Author: xiyangliang
 */

#ifndef SAG_FYBBR_ROUTING_HELPER_H
#define SAG_FYBBR_ROUTING_HELPER_H

#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_helper.h"

namespace ns3 {

class Sag_Fybbr_Routing_Helper: public SAGRoutingHelper {
public:
	static TypeId GetTypeId(void);
    Sag_Fybbr_Routing_Helper();
    Sag_Fybbr_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

};

} // namespace ns3

#endif /* SAG_OPEN_SHORTEST_PATH_FIRST_HELPER_H */
