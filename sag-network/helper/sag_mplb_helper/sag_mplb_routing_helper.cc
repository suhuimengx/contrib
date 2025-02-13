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


#include "sag_mplb_routing_helper.h"
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (Sag_MPLB_Routing_Helper);
TypeId Sag_MPLB_Routing_Helper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Sag_MPLB_Routing_Helper")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<Sag_MPLB_Routing_Helper>()
    ;
    return tid;
}

Sag_MPLB_Routing_Helper::Sag_MPLB_Routing_Helper()
{
	m_factory.SetTypeId ("ns3::mplb::MultiPathLoadBalance");
	std::cout << "Set up Sag_MPLB_Routing_Helper" << std::endl;
}

Sag_MPLB_Routing_Helper::Sag_MPLB_Routing_Helper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	m_factory.SetTypeId ("ns3::mplb::MultiPathLoadBalance");
    std::cout << "Set up Sag_MPLB_Routing_Helper" << std::endl;

}




}
