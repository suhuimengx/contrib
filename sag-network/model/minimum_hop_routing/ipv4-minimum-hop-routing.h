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

#ifndef IPV4_MINIMUM_HOP_ROUTING
#define IPV4_MINIMUM_HOP_ROUTING

#include <list>
#include <utility>
#include <stdint.h>

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/arbiter.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/route_trace_tag.h"
#include "ns3/walker-constellation-structure.h"
#include "ns3/sag_routing_protocal.h"

namespace ns3 {

class Ipv4MinimumHopRouting : public SAGRoutingProtocal
{
public:
    static TypeId GetTypeId (void);

    // constructor
    Ipv4MinimumHopRouting ();


};

} // Namespace ns3

#endif /* IPV4_MINIMUM_HOP_ROUTING */
