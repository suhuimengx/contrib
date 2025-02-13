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

#include "per_tag.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PerTag");

NS_OBJECT_ENSURE_REGISTERED (PerTag);

TypeId
PerTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::PerTag")
            .SetParent<Tag> ()
            .SetGroupName("BasicSim")
            .AddConstructor<PerTag> ()
    ;
    return tid;
}

PerTag::PerTag ()
  : m_per (0.0)
{
  NS_LOG_FUNCTION (this);
}

void
PerTag::SetPer (double per)
{
  NS_LOG_FUNCTION (this << per);
  m_per = per;
}

double
PerTag::GetPer (void) const
{
  NS_LOG_FUNCTION (this);
  return m_per;
}

TypeId
PerTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
PerTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(per=" << m_per << ")";
}

uint32_t
PerTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return sizeof (double);
}

void
PerTag::Serialize (TagBuffer start) const
{
  NS_LOG_FUNCTION (this << &start);
  start.WriteDouble (m_per);
}

void
PerTag::Deserialize (TagBuffer start)
{
  NS_LOG_FUNCTION (this << &start);
  m_per = start.ReadDouble ();
}

} // namespace ns3





