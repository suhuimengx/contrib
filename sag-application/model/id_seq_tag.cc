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

#include "id_seq_tag.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IdSeqTag");

NS_OBJECT_ENSURE_REGISTERED (IdSeqTag);

TypeId
IdSeqTag::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::IdSeqTag")
            .SetParent<Tag> ()
            .SetGroupName("BasicSim")
            .AddConstructor<IdSeqTag> ()
    ;
    return tid;
}

IdSeqTag::IdSeqTag ()
  : m_id (0),
    m_seq (0)
{
  NS_LOG_FUNCTION (this);
}

void
IdSeqTag::SetId (uint64_t id)
{
    NS_LOG_FUNCTION (this << id);
    m_id = id;
}

uint64_t
IdSeqTag::GetId (void) const
{
    NS_LOG_FUNCTION (this);
    return m_id;
}

void
IdSeqTag::SetSeq (uint64_t seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_seq = seq;
}

uint64_t
IdSeqTag::GetSeq (void) const
{
  NS_LOG_FUNCTION (this);
  return m_seq;
}

TypeId
IdSeqTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
IdSeqTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(id=" << m_id << ", seq=" << m_seq << ")";
}

uint32_t
IdSeqTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 8+8;
}

void
IdSeqTag::Serialize (TagBuffer start) const
{
  NS_LOG_FUNCTION (this << &start);
  start.WriteU64 (m_id);
  start.WriteU64 (m_seq);
}

void
IdSeqTag::Deserialize (TagBuffer start)
{
  NS_LOG_FUNCTION (this << &start);
  m_id = start.ReadU64 ();
  m_seq = start.ReadU64 ();
}

} // namespace ns3





