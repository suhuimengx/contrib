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

#ifndef SAG_ROUTING_HELPER_IPV6_H
#define SAG_ROUTING_HELPER_IPV6_H

#include "ns3/log.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/basic-simulation.h"
//#include "ns3/topology-satellite-network.h"
#include "ns3/sag_routing_protocal_ipv6.h"
#include "ns3/abort.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/string.h"
#include "ns3/exp-util.h"
#include "ns3/arbiter-single-forward.h"

//class TopologySatelliteNetwork;

namespace ns3 {

class SAGRoutingHelperIPv6 : public Ipv6RoutingHelper
{
public:
    static TypeId GetTypeId(void);
    SAGRoutingHelperIPv6 ();
	~SAGRoutingHelperIPv6 ();
    SAGRoutingHelperIPv6 (const SAGRoutingHelperIPv6 &);
    SAGRoutingHelperIPv6* Copy (void) const;


    /**
	 * Set the attribute value of the routing protocol.
	 *
	 * \param name       Attribute name
	 * \param value      Attribute value
	 *
	 */
    void Set (std::string name, const AttributeValue &value);
	void SetTopologyHandle(std::vector<Ptr<Constellation>> constellations);
	virtual Ptr<Ipv6RoutingProtocol> Create (Ptr<Node> node) const;


    /**
	 * Initialize arbiter for each node.
	 *
	 * \param basicSimulation		BasicSimulation pointer
	 * \param nodes      All the nodes
	 *
	 */
    virtual void InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

    /**
   	 * Set topology handle.
   	 *
   	 * \param topology		TopologySatelliteNetwork pointer
   	 *
   	 */
    //virtual void SetTopologyHandle(Ptr<TopologySatelliteNetwork> topology);

    /**
   	 * Initial empty forwarding state.
   	 *
   	 * \return		empty forwarding state
   	 *
   	 */
    // virtual std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();

    /**
	 * Maintain periodic forwarding status updates.
	 *
	 * \param t		Starting time
	 *
	 */
    virtual void UpdateForwardingState(int32_t t);

    /**
	 * Update the arbiter routing table.
	 *
	 * \param cur_node		Current node id
	 * \param dst_node		Destination node id
	 * \param next_hop		Next hop node id
	 * \param outInterface		Output interface index
	 * \param inInterface		INput interface index
	 *
	 */
    virtual void UpdateRoutingTable(int32_t cur_node, int32_t dst_node, int32_t next_hop, int32_t outInterface, int32_t inInterface);

    //SAGRoutingHelperIPv6 &operator = (const SAGRoutingHelperIPv6 &);

	void StoreIPv6AddresstoNodeId();
    virtual void UpdateIpv6AddresstoNodeId();

    void SetObjectNameString(std::string objectToBeInstall);

    std::string GetObjectNameString();

    TypeId GetObjectFactoryTypeId() const;

protected:
    // Parameters
    ObjectFactory m_factory;

    Ptr<BasicSimulation> m_basicSimulation;
    std::vector<Ptr<Constellation>>  m_constellations;
    NodeContainer m_nodes;
    NodeContainer m_satellite;
    NodeContainer m_groundStation;
    int64_t m_dynamicStateUpdateIntervalNs;
    std::vector<Ptr<ArbiterSingleForward>> m_arbiters;

    std::map<IPv6AddressBuf, uint32_t, IPv6AddressBufComparator> m_ipv6_to_node_id;
    std::string m_objectToBeInstall;
};

} // namespace ns3

#endif /* SAG_ROUTING_HELPER_IPV6_H */
