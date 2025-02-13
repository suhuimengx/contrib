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
#include "../bgp_routing_helper/bgp-routing-helper.h"

#include "ns3/bgp-routing.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3
{
NS_OBJECT_ENSURE_REGISTERED (BGPRoutingHelper);
TypeId BGPRoutingHelper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::BGPRoutingHelper")
            .SetParent<SAGRoutingHelper> ()
            .SetGroupName("SagRouting")
            .AddConstructor<BGPRoutingHelper>()
    ;
    return tid;
}


BGPRoutingHelper::BGPRoutingHelper()
{
	m_factory.SetTypeId ("ns3::BgpRouting");
}

}
