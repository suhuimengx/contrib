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


#include "ns3/sag_open_shortest_path_first_helper.h"
#include "ns3/open_shortest_path_first.h"
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (Sag_Open_Shortest_Path_First_Helper);
TypeId Sag_Open_Shortest_Path_First_Helper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Sag_Open_Shortest_Path_First_Helper")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<Sag_Open_Shortest_Path_First_Helper>()
    ;
    return tid;
}

Sag_Open_Shortest_Path_First_Helper::Sag_Open_Shortest_Path_First_Helper()
{
	m_factory.SetTypeId ("ns3::ospf::Open_Shortest_Path_First");
}

Sag_Open_Shortest_Path_First_Helper::Sag_Open_Shortest_Path_First_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::ospf::Open_Shortest_Path_First");

}

Ptr<Ipv4RoutingProtocol> Sag_Open_Shortest_Path_First_Helper::Create (Ptr<Node> node) const
{
	//	return m_factory.Create<Ipv4RoutingProtocol> ();
	Ptr<Ipv4RoutingProtocol> agent = m_factory.Create<Ipv4RoutingProtocol> ();
	node->AggregateObject (agent);
	return agent;
}

void Sag_Open_Shortest_Path_First_Helper::InitializeArbiter(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes){

	SAGRoutingHelper::InitializeArbiter(basicSimulation, nodes);
	//m_dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
	//UpdateForwardingState(0);

}

void Sag_Open_Shortest_Path_First_Helper::UpdateForwardingState(int32_t t){

}




}
