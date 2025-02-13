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
#include "ns3/ospf-packet.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace ospf {

//NS_OBJECT_ENSURE_REGISTERED (TypeHeader);
//
//TypeHeader::TypeHeader (MessageType t)
//  : m_type (t),
//    m_valid (true)
//{
//}
//
//TypeId
//TypeHeader::GetTypeId ()
//{
//  static TypeId tid = TypeId ("ns3::ospf::TypeHeader")
//    .SetParent<Header> ()
//    .SetGroupName ("OSPF")
//    .AddConstructor<TypeHeader> ()
//  ;
//  return tid;
//}
//
//TypeId
//TypeHeader::GetInstanceTypeId () const
//{
//  return GetTypeId ();
//}
//
//uint32_t
//TypeHeader::GetSerializedSize () const
//{
//  return 1;
//}
//
//void
//TypeHeader::Serialize (Buffer::Iterator i) const
//{
//  i.WriteU8 ((uint8_t) m_type);
//}
//
//uint32_t
//TypeHeader::Deserialize (Buffer::Iterator start)
//{
//  Buffer::Iterator i = start;
//  uint8_t type = i.ReadU8 ();
//  m_valid = true;
//  switch (type)
//    {
//    case OSPFTYPE_HELLO:
//    case OSPFTYPE_DBD:
//    case OSPFTYPE_LSR:
//    case OSPFTYPE_LSU:
//    case OSPFTYPE_LSAck:
//      {
//        m_type = (MessageType) type;
//        break;
//      }
//    default:
//      m_valid = false;
//    }
//  uint32_t dist = i.GetDistanceFrom (start);
//  NS_ASSERT (dist == GetSerializedSize ());
//  return dist;
//}
//
//void
//TypeHeader::Print (std::ostream &os) const
//{
//  switch (m_type)
//    {
//    case OSPFTYPE_HELLO:
//      {
//        os << "HELLO";
//        break;
//      }
//    case OSPFTYPE_DBD:
//      {
//        os << "DBD";
//        break;
//      }
//    case OSPFTYPE_LSR:
//      {
//        os << "LSR";
//        break;
//      }
//    case OSPFTYPE_LSU:
//      {
//        os << "LSU";
//        break;
//      }
//    case OSPFTYPE_LSAck:
//          {
//            os << "LSAck";
//            break;
//          }
//    default:
//      os << "UNKNOWN_TYPE";
//    }
//}
//
//bool
//TypeHeader::operator== (TypeHeader const & o) const
//{
//  return (m_type == o.m_type && m_valid == o.m_valid);
//}
//
//std::ostream &
//operator<< (std::ostream & os, TypeHeader const & h)
//{
//  h.Print (os);
//  return os;
//}


//-----------------------------------------------------------------------------
// OSPF Packet Header
//-----------------------------------------------------------------------------

OspfHeader::OspfHeader (uint16_t packetLength, MessageType type, Ipv4Address routerID, Ipv4Address areaID,
		   	   	   	   uint16_t checkSum, uint16_t auType, uint32_t authentication1, uint32_t authentication2)
  : m_version (2),
	m_type (type),
	m_packetLength (packetLength),
	m_routerID (routerID),
	m_areaID (areaID),
	m_checkSum (checkSum),
	m_auType (auType),
	m_authentication1 (authentication1),
	m_authentication2 (authentication2)
{

}

NS_OBJECT_ENSURE_REGISTERED (OspfHeader);

TypeId
OspfHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ospf::OspfHeader")
    .SetParent<Header> ()
    .SetGroupName ("OSPF")
    .AddConstructor<OspfHeader> ()
  ;
  return tid;
}

TypeId
OspfHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
OspfHeader::GetSerializedSize () const
{
  return 24;
}

void
OspfHeader::Serialize (Buffer::Iterator i) const
{
	i.WriteU8(m_version);
	i.WriteU8((uint8_t)m_type);
	i.WriteHtonU16 (m_packetLength);
	WriteTo (i, m_routerID);
	WriteTo (i, m_areaID);
	i.WriteHtonU16 (m_checkSum);
	i.WriteHtonU16 (m_auType);
	i.WriteHtonU32 (m_authentication1);
	i.WriteHtonU32 (m_authentication2);
}

uint32_t
OspfHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_version = i.ReadU8();
  uint8_t type = i.ReadU8 ();
	switch (type)
	  {
	  case OSPFTYPE_HELLO:
	  case OSPFTYPE_DBD:
	  case OSPFTYPE_LSR:
	  case OSPFTYPE_LSU:
	  case OSPFTYPE_LSAck:
		{
		  m_type = (MessageType) type;
		  break;
		}
	  }
  m_packetLength = i.ReadNtohU16 ();
  ReadFrom (i, m_routerID);
  ReadFrom (i, m_areaID);
  m_checkSum = i.ReadNtohU16 ();
  m_auType = i.ReadNtohU16 ();
  m_authentication1 = i.ReadNtohU32 ();
  m_authentication2 = i.ReadNtohU32 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
OspfHeader::Print (std::ostream &os) const
{

}

bool
OspfHeader::operator== (OspfHeader const & o ) const
{
  return m_packetLength == o.m_packetLength;
}

//-----------------------------------------------------------------------------
// OSPF Packet Header
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Hello
//-----------------------------------------------------------------------------


NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ospf::HelloHeader")
    .SetParent<Header> ()
    .SetGroupName ("OSPF")
    .AddConstructor<HelloHeader> ()
  ;
  return tid;
}

HelloHeader::HelloHeader(uint32_t mask, Time helloInterval, uint8_t options, uint8_t rtrPri,
		  Time rtrDeadInterval, Ipv4Address dr, Ipv4Address bdr, std::vector<Ipv4Address> neighors)
  : m_mask (mask),
	m_options (options),
	m_rtrPri (rtrPri),
	m_dr (dr),
	m_bdr (bdr),
	m_neighors (neighors)
{
	m_helloInterval = uint16_t (helloInterval.GetSeconds());
	m_rtrDeadInterval = uint32_t (rtrDeadInterval.GetSeconds());
	//m_nbNum = neighors.size();
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return (20 + m_neighors.size()*4);
}

void
HelloHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtonU32 (m_mask);
  i.WriteHtonU16 (m_helloInterval);
  i.WriteU8 (m_options);
  i.WriteU8 (m_rtrPri);
  i.WriteHtonU32 (m_rtrDeadInterval);
  WriteTo(i,m_dr);
  WriteTo(i,m_bdr);
  //i.WriteU8 (m_nbNum);
  for(Ipv4Address n : m_neighors){
	  WriteTo(i,n);
  }
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_mask = i.ReadNtohU32 ();
  m_helloInterval = i.ReadNtohU16();
  m_options = i.ReadU8 ();
  m_rtrPri = i.ReadU8 ();
  m_rtrDeadInterval = i.ReadNtohU32 ();
  ReadFrom(i,m_dr);
  ReadFrom(i,m_bdr);
  //m_nbNum = i.ReadU8 ();
  NS_ASSERT(i.GetRemainingSize()%4 == 0);
  int d = i.GetRemainingSize();

  int nbNum = (d) / 4;
//  if(nbNum!=0){
//	  std::cout<<nbNum<<std::endl;
//  }

  for(int j = 0; j < nbNum; j++){
	  Ipv4Address ad;
	  ReadFrom(i,ad);
	  m_neighors.push_back(ad);
  }

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
HelloHeader::Print (std::ostream &os) const
{

}

bool
HelloHeader::operator== (HelloHeader const & o ) const
{
  return m_mask == o.m_mask;
}

//-----------------------------------------------------------------------------
// Hello
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// DD
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (DDHeader);

TypeId
DDHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ospf::DDHeader")
    .SetParent<Header> ()
    .SetGroupName ("OSPF")
    .AddConstructor<DDHeader> ()
  ;
  return tid;
}

DDHeader::DDHeader(uint16_t mtu, uint8_t options, uint8_t flags, uint32_t seqNum, std::vector<LSAHeader> lsaHeaders)
	: m_mtu (mtu),
	  m_options (options),
	  m_flags (flags),
	  m_seqNum (seqNum),
	  m_lsaHeaders (lsaHeaders)
{
	//m_lsNum = m_lsaHeaders.size();
//	for(auto x = 0; x < m_lsaHeaders.size(); x++){
//		m_lsaHeaders[x].SetPacketLength(this->GetSerializedSize());
//	}
}

TypeId
DDHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
DDHeader::GetSerializedSize () const
{
	uint32_t t = 0;
	for(auto lsaHeader : m_lsaHeaders){
		t += lsaHeader.GetSerializedSize();
	}
  return (8 + t);
}

void
DDHeader::Serialize (Buffer::Iterator i) const
{
	i.WriteHtonU16(m_mtu);
	i.WriteU8(m_options);
	i.WriteU8(m_flags);
	i.WriteHtonU32(m_seqNum);
	for(auto lsaHeader : m_lsaHeaders){
		lsaHeader.Serialize(i);
		i.Next(lsaHeader.GetSerializedSize());
	}

}

uint32_t
DDHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_mtu = i.ReadNtohU16();
	m_options = i.ReadU8();
	m_flags = i.ReadU8();
	m_seqNum = i.ReadNtohU32();

	LSAHeader lstemp = LSAHeader();
	NS_ASSERT(i.GetRemainingSize() % lstemp.GetSerializedSize() == 0);
	int d = i.GetRemainingSize();

	int lsNum = (d) / lstemp.GetSerializedSize();

	for(int j = 0; j < lsNum; j++){
		LSAHeader ls = LSAHeader();
		ls.Deserialize(i);
		m_lsaHeaders.push_back(ls);
		i.Next(ls.GetSerializedSize());
	}

	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}

void
DDHeader::Print (std::ostream &os) const
{

}




//-----------------------------------------------------------------------------
// DD
//-----------------------------------------------------------------------------






}
}
