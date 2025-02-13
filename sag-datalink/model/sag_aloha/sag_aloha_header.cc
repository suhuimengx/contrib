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

#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "sag_aloha_header.h"
#include "ns3/address-utils.h"

namespace ns3 {
namespace aloha {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t)
  : m_type (t),
    m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::aloha::TypeHeader")
    .SetParent<Header> ()
    .SetGroupName ("aloha")
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case ALOHA_DATA:
    case ALOHA_ACK:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{

}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}


NS_LOG_COMPONENT_DEFINE ("SAGAlohaHeader");

NS_OBJECT_ENSURE_REGISTERED (SAGAlohaHeader);

SAGAlohaHeader::SAGAlohaHeader ()
{
}

SAGAlohaHeader::~SAGAlohaHeader ()
{
}

TypeId
SAGAlohaHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::aloha::SAGAlohaHeader")
    .SetParent<Header> ()
    .SetGroupName ("aloha")
    .AddConstructor<SAGAlohaHeader> ()
  ;
  return tid;
}

TypeId
SAGAlohaHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
SAGAlohaHeader::Print (std::ostream &os) const
{

}

uint32_t
SAGAlohaHeader::GetSerializedSize (void) const
{
  return 22;
}

void
SAGAlohaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU16 (m_protocol);
  WriteTo(start, m_destination);
  WriteTo(start, m_source);
  start.WriteHtonU64(m_packetSeq);
}

uint32_t
SAGAlohaHeader::Deserialize (Buffer::Iterator start)
{
  m_protocol = start.ReadNtohU16 ();
  ReadFrom(start, m_destination);
  ReadFrom(start, m_source);
  m_packetSeq = start.ReadNtohU64();
  return GetSerializedSize ();
}

void
SAGAlohaHeader::SetProtocol (uint16_t protocol)
{
  m_protocol=protocol;
}

uint16_t
SAGAlohaHeader::GetProtocol (void)
{
  return m_protocol;
}

void
SAGAlohaHeader::SetSource(Mac48Address source)
{
    m_source = source;
}

Mac48Address
SAGAlohaHeader::GetSource() const
{
    return m_source;
}

void
SAGAlohaHeader::SetDestination(Mac48Address dst)
{
    m_destination = dst;
}

Mac48Address
SAGAlohaHeader::GetDestination() const
{
    return m_destination;
}

void
SAGAlohaHeader::SetPacketSequence(uint64_t seq){

	m_packetSeq = seq;

}

uint64_t
SAGAlohaHeader::GetPacketSequence(){

	return m_packetSeq;

}

}
} // namespace ns3




