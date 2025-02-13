/*
 * sag_fybbr_routing_helper.cc
 *
 *  Created on: 2023年1月31日
 *      Author: xiyangliang
 */


#include "ns3/sag_fybbr_routing_helper.h"
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (Sag_Fybbr_Routing_Helper);
TypeId Sag_Fybbr_Routing_Helper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Sag_Fybbr_Routing_Helper")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<Sag_Fybbr_Routing_Helper>()
    ;
    return tid;
}

Sag_Fybbr_Routing_Helper::Sag_Fybbr_Routing_Helper()
{
	m_factory.SetTypeId ("ns3::fybbr::Fybbr_Rout");
	std::cout << "Set up sag_fybbr_routing_helper" << std::endl;
}

Sag_Fybbr_Routing_Helper::Sag_Fybbr_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::fybbr::Fybbr_Rout");
    std::cout << "Set up sag_fybbr_routing_helper" << std::endl;
}

}
