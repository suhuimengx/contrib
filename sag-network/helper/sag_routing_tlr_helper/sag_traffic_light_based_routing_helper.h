

#ifndef SAG_TRAFFIC_LIGHT_BASED_ROUTING_HELPER_H
#define SAG_TRAFFIC_LIGHT_BASED_ROUTING_HELPER_H

#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_helper.h"

namespace ns3 {

	class Sag_Traffic_Light_Based_Routing_Helper : public SAGRoutingHelper {
	public:
		static TypeId GetTypeId(void);
		Sag_Traffic_Light_Based_Routing_Helper();
		Sag_Traffic_Light_Based_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

	};

} // namespace ns3

#endif 