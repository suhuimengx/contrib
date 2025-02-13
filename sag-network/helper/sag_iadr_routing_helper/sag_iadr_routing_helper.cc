/*
 * sag_iadr_routing_helper.cc
 *
 *  Created on: 2023年1月31日
 *      Author: xiyangliang
 */


#include "ns3/sag_iadr_routing_helper.h"
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (Sag_Iadr_Routing_Helper);
TypeId Sag_Iadr_Routing_Helper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Sag_Iadr_Routing_Helper")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<Sag_Iadr_Routing_Helper>()
    ;
    return tid;
}

Sag_Iadr_Routing_Helper::Sag_Iadr_Routing_Helper()
{
	m_factory.SetTypeId ("ns3::iadr::Iadr_Rout");
	std::cout << "Set up sag_iadr_routing_helper" << std::endl;
}

Sag_Iadr_Routing_Helper::Sag_Iadr_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::iadr::Iadr_Rout");
    std::cout << "Set up sag_iadr_routing_helper" << std::endl;
}

}
