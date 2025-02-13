//
// Created by Mengy on 2023/1/30.
//

#include "ns3/iadr_link_state_packet.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"


namespace ns3 {
class Ipv4Address;
namespace iadr {


//-----------------------------------------------------------------------------
// LSA Packer Header
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSAHeader);

TypeId
LSAHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::iadr::LSAHeader")
            .SetParent<Header> ()
            .SetGroupName ("IADR")
            .AddConstructor<LSAHeader> ()
    ;
    return tid;
}

LSAHeader::LSAHeader(uint32_t LSage, uint8_t options, Ipv4Address LinkStateID,
                     Ipv4Address AdvertisingRouter, uint32_t LSsequence,
                     uint8_t checkSum, uint8_t PacketLength, uint32_t addedTime)
        : m_LSage (LSage),
          m_options (options),
          m_LinkStateID (LinkStateID),
          m_AdvertisingRouter (AdvertisingRouter),
          m_LSsequence (LSsequence),
          m_checksum (checkSum),
          m_PacketLength (PacketLength),
		  m_addTime(addedTime)
{
    m_LSAtype =  1;
}


TypeId
LSAHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSAHeader::GetSerializedSize () const
{
    return 23;
}

void
LSAHeader::Serialize (Buffer::Iterator i) const
{
    i.WriteHtonU32 (m_LSage);
    i.WriteU8 (m_options);
    WriteTo (i, m_LinkStateID);
    WriteTo (i, m_AdvertisingRouter);
    i.WriteHtonU32 (m_LSsequence);
    i.WriteU8 (m_checksum);
    i.WriteU8 (m_PacketLength);
    i.WriteHtonU32(m_addTime);
}

uint32_t
LSAHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_LSage = i.ReadNtohU32 ();
    m_options = i.ReadU8 ();
    ReadFrom (i, m_LinkStateID);
    ReadFrom (i, m_AdvertisingRouter);
    m_LSsequence = i.ReadNtohU32 ();
    m_checksum = i.ReadU8 ();
    m_PacketLength = i.ReadU8();
    m_addTime = i.ReadNtohU32 ();

    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSAHeader::Print (std::ostream &os) const
{

}

bool
LSAHeader::operator== (LSAHeader const & o ) const
{
//	if(m_checksum == o.m_checksum)
//	{
//		if(m_LSsequence == o.m_LSsequence)
//		{
//			return true;
//		}
//	}
	if(m_LSAtype == o.m_LSAtype && m_LinkStateID == o.m_LinkStateID && m_AdvertisingRouter == o.m_AdvertisingRouter){
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
// LSA Packet Header
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//LSA Packet
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSAPacket);

TypeId
LSAPacket::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::iadr::LSAPacket")
            .SetParent<Header> ()
            .SetGroupName ("IADR")
            .AddConstructor<LSAPacket> ()
    ;
    return tid;
}

LSAPacket::LSAPacket(uint8_t VEB, uint8_t linkn, std::vector<LSALinkData> LSALinkDatas)
        : m_VEB (VEB),
          m_linkn (linkn),
          m_LSALinkDatas (LSALinkDatas)
{

}


TypeId
LSAPacket::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSAPacket::GetSerializedSize () const
{
	uint32_t t = 0;
	for(uint32_t n = 0; n < m_linkn; n++){
		t += m_LSALinkDatas[n].GetSerializedSize();
	}
    return 2 + t;
}

void
LSAPacket::Serialize (Buffer::Iterator i) const
{
    i.WriteU8 (m_VEB);
    i.WriteU8 (m_linkn);
    for(uint8_t n = 0; n < m_linkn; n++)
    {
        m_LSALinkDatas[n].Serialize(i);
        i.Next(m_LSALinkDatas[n].GetSerializedSize());
    }
}

uint32_t
LSAPacket::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_VEB = i.ReadU8 ();
    m_linkn = i.ReadU8 ();
    for(uint8_t n = 0; n < m_linkn; n++)
    {
    	LSALinkData lsL = LSALinkData();
    	lsL.Deserialize(i);
		i.Next(lsL.GetSerializedSize());
		m_LSALinkDatas.push_back(lsL);
    }

    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSAPacket::Print (std::ostream &os) const
{

}

bool
LSAPacket::operator== (LSAPacket const & o ) const
{
	if( m_VEB == o.m_VEB)
	{
		if(m_linkn == o.m_linkn)
		{
			std::vector<LSALinkData> llds = o.m_LSALinkDatas;
			bool equalOrNot = true;
			for(uint32_t i = 0; i < m_linkn; i++){
				if(!(llds.at(i) == m_LSALinkDatas.at(i))){
					equalOrNot = false;
					break;
				}
			}
			return equalOrNot;

		}
	}
    return false;
}

//-----------------------------------------------------------------------------
//LSA Packet
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//LSA Link Data Packet
//-----------------------------------------------------------------------------
LSALinkData::LSALinkData (Ipv4Address linkID, Ipv4Address linkdata, Ipv4Address neighborlinkID, uint8_t metric, uint16_t metric2, uint8_t metric3)
        :  m_linkID (linkID),
           m_linkdata (linkdata),
		   m_neighborlinkID (neighborlinkID),
           m_metric(metric),
		   m_metric2(metric2),
		   m_metric3(metric3)

{
    m_linktype = 1;
    m_TOSn = 0;
    m_TOS = 0;
    m_TOSmetric = 0;
}

NS_OBJECT_ENSURE_REGISTERED (LSALinkData);

TypeId
LSALinkData::GetTypeId () {
    static TypeId tid = TypeId("ns3::iadr::LSALinkData")
            .SetParent<Header>()
            .SetGroupName("IADR")
            .AddConstructor<LSALinkData>();
    return tid;
}

TypeId
LSALinkData::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSALinkData::GetSerializedSize () const
{
    return 16;
}

void
LSALinkData::Serialize (Buffer::Iterator i) const
{
    WriteTo (i, m_linkID);
    WriteTo (i, m_linkdata);
    WriteTo (i, m_neighborlinkID);
    i.WriteU8 (m_metric);
    i.WriteU16 (m_metric2);
    i.WriteU8 (m_metric3);
}

uint32_t
LSALinkData::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    ReadFrom (i, m_linkID);
    ReadFrom (i, m_linkdata);
    ReadFrom (i, m_neighborlinkID);
    m_metric = i.ReadU8 ();
    m_metric2 = i.ReadU16 ();
    m_metric3 = i.ReadU8 ();

    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSALinkData::Print (std::ostream &os) const
{

}

bool
LSALinkData::operator== (LSALinkData const & o ) const
{
    if(m_linkID == o.m_linkID)
	{
       if (m_linkdata == o.m_linkdata)
       {
    	   if (m_neighborlinkID == o.m_neighborlinkID)
    	   {
			   if(m_linktype == o.m_linktype)
			   {
				   if(m_TOSn == o.m_TOSn)
				   {
					   if(m_metric == o.m_metric&&m_metric2 == o.m_metric2&&m_metric3 == o.m_metric3)
					   {
						   if(m_TOS == o.m_TOS)
						   {
							   if(m_TOSmetric == o.m_TOSmetric)
							   {
								   return true;
							   }
						   }
					   }
				   }
			   }
    	   }
       }
    }
    return false;
}


NS_OBJECT_ENSURE_REGISTERED (LSUHeader);

TypeId
LSUHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::iadr::LSUHeader")
            .SetParent<Header> ()
            .SetGroupName ("IADR")
            .AddConstructor<LSUHeader> ()
    ;
    return tid;
}

LSUHeader::LSUHeader(std::vector<std::pair<LSAHeader,LSAPacket>> LSAs)
        : m_LSAs (LSAs)
{
	m_LSAn = m_LSAs.size();
}


TypeId
LSUHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSUHeader::GetSerializedSize () const
{
    uint32_t LSAheadern = 0;
    uint32_t LSApacketn = 0;

    for(int n = 0; n < int(m_LSAs.size()); n++)
    {
    	LSAheadern += m_LSAs[n].first.GetSerializedSize ();
        LSApacketn += m_LSAs[n].second.GetSerializedSize ();
    }
    return 4 + LSApacketn + LSAheadern;
    //return 4 + LSAheadern;
}

void
LSUHeader::Serialize (Buffer::Iterator i) const
{
    i.WriteHtonU32 (m_LSAn);
    for(int n = 0; n < int(m_LSAs.size()); n++)
    {
        m_LSAs[n].first.Serialize(i);
        i.Next(m_LSAs[n].first.GetSerializedSize());
        m_LSAs[n].second.Serialize(i);
        i.Next(m_LSAs[n].second.GetSerializedSize());
    }
}

uint32_t
LSUHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_LSAn = i.ReadNtohU32 ();
    for(uint32_t n = 0; n < m_LSAn; n++)
    {
    	LSAHeader lsH = LSAHeader();
    	lsH.Deserialize(i);
		i.Next(lsH.GetSerializedSize());
    	LSAPacket lsP = LSAPacket();
    	lsP.Deserialize(i);
    	i.Next(lsP.GetSerializedSize());
    	m_LSAs.push_back(std::make_pair(lsH, lsP));
    }

    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSUHeader::Print (std::ostream &os) const
{

}

bool
LSUHeader::operator== (LSUHeader const & o ) const
{
    return true;
}

}
}
