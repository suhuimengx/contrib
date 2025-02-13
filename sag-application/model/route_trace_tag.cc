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
#include "ns3/log.h"
#include "route_trace_tag.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RouteTraceTag");

NS_OBJECT_ENSURE_REGISTERED (RouteTraceTag);

RouteTraceTag::RouteTraceTag (uint8_t hops, std::vector<uint32_t> nodeId)
{
  NS_LOG_FUNCTION (this);
  m_hops = hops;
  m_nodeId = nodeId;
}

std::vector<uint32_t>
RouteTraceTag::GetRouteTrace (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nodeId;
}

TypeId
RouteTraceTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RouteTraceTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<RouteTraceTag> ()
  ;
  return tid;
}
TypeId
RouteTraceTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
RouteTraceTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 1 + m_nodeId.size() * 4;
}
void
RouteTraceTag::Serialize (TagBuffer i) const
{
  NS_LOG_FUNCTION (this << &i);
  i.WriteU8 (m_hops);
  for(auto routeNode : m_nodeId){
	  i.WriteU32(routeNode);
  }
}
void
RouteTraceTag::Deserialize (TagBuffer i)
{
  NS_LOG_FUNCTION (this << &i);
  m_hops = i.ReadU8 ();
  for(uint8_t n = 0; n < m_hops; n++){
	  m_nodeId.push_back(i.ReadU32());
  }
}
void
RouteTraceTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  //os << "Ttl=" << (uint32_t) m_ttl;
}

}

