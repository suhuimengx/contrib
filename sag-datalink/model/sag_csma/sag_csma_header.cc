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
#include "ns3/address-utils.h"
#include "sag_csma_header.h"

namespace ns3 {
namespace csma {

NS_LOG_COMPONENT_DEFINE ("SAGCsmaHeader");

NS_OBJECT_ENSURE_REGISTERED (SAGCsmaHeader);

SAGCsmaHeader::SAGCsmaHeader ()
{
}

SAGCsmaHeader::~SAGCsmaHeader ()
{
}

TypeId
SAGCsmaHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::csma::SAGCsmaHeader")
    .SetParent<Header> ()
    .SetGroupName ("Csma")
    .AddConstructor<SAGCsmaHeader> ()
  ;
  return tid;
}

TypeId
SAGCsmaHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
SAGCsmaHeader::Print (std::ostream &os) const
{

}

uint32_t
SAGCsmaHeader::GetSerializedSize (void) const
{
  return 22;
}

void
SAGCsmaHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU16 (m_protocol);
  WriteTo(start, m_destination);
  WriteTo(start, m_source);
  start.WriteHtonU64(m_packetSeq);
}

uint32_t
SAGCsmaHeader::Deserialize (Buffer::Iterator start)
{
  m_protocol = start.ReadNtohU16 ();
  ReadFrom(start, m_destination);
  ReadFrom(start, m_source);
  m_packetSeq = start.ReadNtohU64();
  return GetSerializedSize ();
}

void
SAGCsmaHeader::SetProtocol (uint16_t protocol)
{
  m_protocol=protocol;
}

uint16_t
SAGCsmaHeader::GetProtocol (void)
{
  return m_protocol;
}

void
SAGCsmaHeader::SetSource(Mac48Address source)
{
    m_source = source;
}

Mac48Address
SAGCsmaHeader::GetSource() const
{
    return m_source;
}

void
SAGCsmaHeader::SetDestination(Mac48Address dst)
{
    m_destination = dst;
}

Mac48Address
SAGCsmaHeader::GetDestination() const
{
    return m_destination;
}

void
SAGCsmaHeader::SetPacketSequence(uint64_t seq){

	m_packetSeq = seq;

}

uint64_t
SAGCsmaHeader::GetPacketSequence(){

	return m_packetSeq;

}

} // namespace csma
} // namespace ns3




