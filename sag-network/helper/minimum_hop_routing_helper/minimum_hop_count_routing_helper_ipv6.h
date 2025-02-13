/*
 * minimum_hop_count_routing_helper_ipv6.h
 *
 *  Created on: 2022年12月27日
 *      Author: root
 */

#ifndef MINIMUM_HOP_COUNT_ROUTING_HELPER_IPv6_H
#define MINIMUM_HOP_COUNT_ROUTING_HELPER_IPv6_H

#include "ns3/ipv6-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_protocal_ipv6.h"
#include "ns3/arbiter.h"
#include "ns3/abort.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/string.h"
#include "ns3/exp-util.h"
#include "ns3/sag_routing_helper_ipv6.h"
#include "ns3/ipv6.h"
//#include "ns3/sag_rtp_constants.h"

namespace ns3 {

class Minimum_Hop_Count_Routing_Helper_IPv6: public SAGRoutingHelperIPv6 {
public:
	static TypeId GetTypeId(void);
	Minimum_Hop_Count_Routing_Helper_IPv6();
	Minimum_Hop_Count_Routing_Helper_IPv6(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);
	void InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

	void UpdateForwardingState(int32_t t);
	int32_t HopCount(int32_t cur_node, int32_t dst_node, Ptr<Constellation> cons);

};

} // namespace ns3



#endif /* MINIMUM_HOP_COUNT_ROUTING_HELPER_IPv6_H */
