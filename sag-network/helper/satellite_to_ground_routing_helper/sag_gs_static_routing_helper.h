/*
 * sag_gs_static_routing_helper.h
 *
 *  Created on: 2022年12月2日
 *      Author: root
 */

#ifndef SAG_GS_STATIC_ROUTING_HELPER_H
#define SAG_GS_STATIC_ROUTING_HELPER_H
#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/sag_routing_protocal.h"
#include "ns3/abort.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/string.h"
#include "ns3/exp-util.h"
#include "ns3/sag_routing_helper.h"
//#include "ns3/sag_rtp_constants.h"


namespace ns3 {

class SAG_GS_Static_Routing_Helper: public SAGRoutingHelper {
public:
	static TypeId GetTypeId(void);
	SAG_GS_Static_Routing_Helper();
	SAG_GS_Static_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);
	virtual void InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);


private:
	virtual void UpdateForwardingState(int64_t t);
	void GSLSwitch(int64_t t);
	void UpdateGSRoutingTable(Ptr<Node> groundStation, Ptr<Node> satellite);
	void UpdateSATRoutingTable(Ptr<Node> groundStation, Ptr<Node> satellite);


};

} // namespace ns3




#endif /* SAG_GS_STATIC_ROUTING_HELPER_H_ */
