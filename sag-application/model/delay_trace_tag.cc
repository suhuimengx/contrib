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
#include "delay_trace_tag.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DelayTraceTag");

NS_OBJECT_ENSURE_REGISTERED (DelayTraceTag);

DelayTraceTag::DelayTraceTag (){
	m_startTimeInMs = 0;
}


DelayTraceTag::DelayTraceTag (uint64_t startTimeInMs)
{
  NS_LOG_FUNCTION (this);
  m_startTimeInMs = startTimeInMs;
}

TypeId
DelayTraceTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DelayTraceTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<DelayTraceTag> ()
  ;
  return tid;
}
TypeId
DelayTraceTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
DelayTraceTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 8;
}
void
DelayTraceTag::Serialize (TagBuffer i) const
{
  NS_LOG_FUNCTION (this << &i);
  i.WriteU64 (m_startTimeInMs);
}
void
DelayTraceTag::Deserialize (TagBuffer i)
{
  NS_LOG_FUNCTION (this << &i);
  m_startTimeInMs = i.ReadU64 ();

}
void
DelayTraceTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  //os << "Ttl=" << (uint32_t) m_ttl;
}

uint64_t
DelayTraceTag::GetStartTime(){
	return m_startTimeInMs;
}

}


