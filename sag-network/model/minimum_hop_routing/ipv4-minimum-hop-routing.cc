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

#define NS_LOG_APPEND_CONTEXT  \
  if (m_ipv4 && m_ipv4->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-minimum-hop-routing.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("Ipv4MinimumHopRouting");

    NS_OBJECT_ENSURE_REGISTERED (Ipv4MinimumHopRouting);

    TypeId
	Ipv4MinimumHopRouting::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::Ipv4MinimumHopRouting")
                .SetParent<SAGRoutingProtocal>()
                .SetGroupName("Internet")
                .AddConstructor<Ipv4MinimumHopRouting>()
				;
        return tid;
    }

    Ipv4MinimumHopRouting::Ipv4MinimumHopRouting()
    :SAGRoutingProtocal()
    {
        NS_LOG_FUNCTION(this);
    }



} // namespace ns3
