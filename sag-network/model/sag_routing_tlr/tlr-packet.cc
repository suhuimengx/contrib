/*
 * tlr-packet.h
 *
 *  Created on: 2023年1月5日
 *      Author: root
 */
#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/tlr-packet.h"

namespace ns3 {
namespace tlr {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t)
  : m_type (t),
    m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::TypeHeader")
    .SetParent<Header> ()
    .SetGroupName ("TLR")
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
    case TLRTYPE_HELLO:
    case TLRTYPE_DBD:
    case TLRTYPE_LSR:
    case TLRTYPE_LSU:
    case TLRTYPE_LSAck:
    case TLRTYPE_NOTIFY:
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
  switch (m_type)
    {
    case TLRTYPE_HELLO:
      {
        os << "HELLO";
        break;
      }
    case TLRTYPE_DBD:
      {
        os << "DBD";
        break;
      }
    case TLRTYPE_LSR:
      {
        os << "LSR";
        break;
      }
    case TLRTYPE_LSU:
      {
        os << "LSU";
        break;
      }
    case TLRTYPE_LSAck:
      {
            os << "LSAck";
            break;
      }
    case TLRTYPE_NOTIFY:
    {
        os << "NOTIFY";
        break;
    }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}


//-----------------------------------------------------------------------------
// TLR Packet Header
//-----------------------------------------------------------------------------

TlrHeader::TlrHeader (uint8_t packetLength, Ipv4Address routerID, Ipv4Address areaID,
		   	   	   	   uint8_t checkSum, uint8_t auType, uint32_t authentication)
  : m_version (2),
	m_packetLength (packetLength),
	m_routerID (routerID),
	m_areaID (areaID),
	m_checkSum (checkSum),
	m_auType (auType),
	m_authentication (authentication)
{

}

NS_OBJECT_ENSURE_REGISTERED (TlrHeader);

TypeId
TlrHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::TlrHeader")
    .SetParent<Header> ()
    .SetGroupName ("TLR")
    .AddConstructor<TlrHeader> ()
  ;
  return tid;
}

TypeId
TlrHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TlrHeader::GetSerializedSize () const
{
  return 10;
}

void
TlrHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_packetLength);
  WriteTo (i, m_routerID);
  WriteTo (i, m_areaID);
  i.WriteU8 (m_checkSum);
}

uint32_t
TlrHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_packetLength = i.ReadU8 ();
  ReadFrom (i, m_routerID);
  ReadFrom (i, m_areaID);
  m_checkSum = i.ReadU8 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TlrHeader::Print (std::ostream &os) const
{

}

bool
TlrHeader::operator== (TlrHeader const & o ) const
{
  return m_packetLength == o.m_packetLength;
}

//-----------------------------------------------------------------------------
// TLR Packet Header
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Hello
//-----------------------------------------------------------------------------


NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::HelloHeader")
    .SetParent<Header> ()
    .SetGroupName ("TLR")
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
	m_helloInterval = uint32_t (helloInterval.GetMilliSeconds ());
	m_rtrDeadInterval = uint32_t (rtrDeadInterval.GetMilliSeconds ());
	m_nbNum = neighors.size();
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return (23 + m_neighors.size()*4);
}

void
HelloHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtonU32 (m_mask);
  i.WriteHtonU32 (m_helloInterval);
  i.WriteU8 (m_options);
  i.WriteU8 (m_rtrPri);
  i.WriteHtonU32 (m_rtrDeadInterval);
  WriteTo(i,m_dr);
  WriteTo(i,m_bdr);
  i.WriteU8 (m_nbNum);
  for(Ipv4Address n : m_neighors){
	  WriteTo(i,n);
  }
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_mask = i.ReadNtohU32 ();
  m_helloInterval = i.ReadNtohU32 ();
  m_options = i.ReadU8 ();
  m_rtrPri = i.ReadU8 ();
  m_rtrDeadInterval = i.ReadNtohU32 ();
  ReadFrom(i,m_dr);
  ReadFrom(i,m_bdr);
  m_nbNum = i.ReadU8 ();
  for(size_t j = 0; j < m_nbNum; j++){
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
// Notify
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(NotifyHeader);

TypeId
NotifyHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::NotifyHeader")
    .SetParent<Header> ()
    .SetGroupName ("TLR")
    .AddConstructor<NotifyHeader> ()
  ;
  return tid;
}

NotifyHeader::NotifyHeader( uint32_t id,uint8_t color)
	:m_id(id),
	 m_color(color)
{
}

TypeId
NotifyHeader::GetInstanceTypeId() const
{
	return GetTypeId();
}

uint32_t
NotifyHeader::GetSerializedSize () const
{
  return 5;
}


void
NotifyHeader::Serialize(Buffer::Iterator i) const
{
	i.WriteHtonU32(m_id);
	i.WriteU8(m_color);

}

uint32_t
NotifyHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_id = i.ReadNtohU32();
	m_color = i.ReadU8();
	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}

void
NotifyHeader::Print(std::ostream& os) const
{

}
//-----------------------------------------------------------------------------
// Notify
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// DD
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (DDHeader);

TypeId
DDHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::tlr::DDHeader")
    .SetParent<Header> ()
    .SetGroupName ("TLR")
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
	m_lsNum = m_lsaHeaders.size();
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
  return (9 + t);
}

void
DDHeader::Serialize (Buffer::Iterator i) const
{
	i.WriteU16(m_mtu);
	i.WriteU8(m_options);
	i.WriteU8(m_flags);
	i.WriteHtonU32(m_seqNum);
	i.WriteU8(m_lsNum);
	for(auto lsaHeader : m_lsaHeaders){
		lsaHeader.Serialize(i);
		i.Next(lsaHeader.GetSerializedSize());
	}

}

uint32_t
DDHeader::Deserialize (Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_mtu = i.ReadU16();
	m_options = i.ReadU8();
	m_flags = i.ReadU8();
	m_seqNum = i.ReadNtohU32();
	m_lsNum = i.ReadU8();
	for(uint8_t j = 0; j < m_lsNum; j++){
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
