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

#include "ns3/ospf_link_state_packet.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"


namespace ns3 {
class Ipv4Address;
namespace ospf {


//-----------------------------------------------------------------------------
// LSA Packer Header
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSAHeader);

TypeId
LSAHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::ospf::LSAHeader")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
            .AddConstructor<LSAHeader> ()
    ;
    return tid;
}

LSAHeader::LSAHeader(uint16_t LSage, uint8_t options, Ipv4Address LinkStateID,
                     Ipv4Address AdvertisingRouter, uint32_t LSsequence,
                     uint16_t checkSum, uint16_t PacketLength, uint32_t addedTime)
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
    return 20;
}

void
LSAHeader::Serialize (Buffer::Iterator i) const
{
    i.WriteHtonU16 (m_LSage);
    i.WriteU8 (m_options);
    i.WriteU8(m_LSAtype);
    WriteTo (i, m_LinkStateID);
    WriteTo (i, m_AdvertisingRouter);
    i.WriteHtonU32 (m_LSsequence);
    //i.WriteHtonU16 (m_checksum);
    i.WriteHtonU16 (m_addTime);
    i.WriteHtonU16 (m_PacketLength);
    //i.WriteHtonU32(m_addTime);
}

uint32_t
LSAHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_LSage = i.ReadNtohU16();
    m_options = i.ReadU8 ();
    m_LSAtype = i.ReadU8 ();
    ReadFrom (i, m_LinkStateID);
    ReadFrom (i, m_AdvertisingRouter);
    m_LSsequence = i.ReadNtohU32 ();
    //m_checksum = i.ReadNtohU16 ();
    m_addTime = i.ReadNtohU16 ();
    m_PacketLength = i.ReadNtohU16();
    //m_addTime = i.ReadNtohU32 ();

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
    static TypeId tid = TypeId ("ns3::ospf::LSAPacket")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
            .AddConstructor<LSAPacket> ()
    ;
    return tid;
}

LSAPacket::LSAPacket(uint8_t VEB, uint16_t linkn, std::vector<LSALinkData> LSALinkDatas)
        : m_VEB (VEB),
          m_linkn (linkn),
          m_LSALinkDatas (LSALinkDatas)
{
	m_zero = 0;
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
    return 4 + t;
}

void
LSAPacket::Serialize (Buffer::Iterator i) const
{
    i.WriteU8 (m_VEB);
    i.WriteU8 (m_zero);
    i.WriteHtonU16 (m_linkn);
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
    m_zero = i.ReadU8 ();
    m_linkn = i.ReadNtohU16();
    for(uint16_t n = 0; n < m_linkn; n++)
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
LSALinkData::LSALinkData (Ipv4Address linkID, Ipv4Address linkdata, uint16_t metric)
        :  m_linkID (linkID),
           m_linkdata (linkdata),
           m_metric(metric)

{
    m_linktype = 1;
    m_TOSn = 0;
    m_TOS = 0;
    m_TOSmetric = 0;
    m_zero = 0;
}

NS_OBJECT_ENSURE_REGISTERED (LSALinkData);

TypeId
LSALinkData::GetTypeId () {
    static TypeId tid = TypeId("ns3::ospf::LSALinkData")
            .SetParent<Header>()
            .SetGroupName("OSPF")
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
    return 12;
}

void
LSALinkData::Serialize (Buffer::Iterator i) const
{
    WriteTo (i, m_linkID);
    WriteTo (i, m_linkdata);
    i.WriteU8 (m_linktype);
    i.WriteU8 (m_TOSn);
    i.WriteHtonU16 (m_metric);
//    i.WriteU8 (m_TOS);
//    i.WriteU8 (m_zero);
//    i.WriteHtonU16 (m_TOSmetric);
}

uint32_t
LSALinkData::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    ReadFrom (i, m_linkID);
    ReadFrom (i, m_linkdata);
    m_linktype = i.ReadU8 ();
    m_TOSn = i.ReadU8 ();
    m_metric = i.ReadNtohU16();
//    m_TOS = i.ReadU8 ();
//    m_zero = i.ReadU8 ();
//    m_TOSmetric = i.ReadNtohU16();

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
    	   if(m_linktype == o.m_linktype)
    	   {
    		   if(m_TOSn == o.m_TOSn)
    		   {
    			   if(m_metric == o.m_metric)
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
    return false;
}

//-----------------------------------------------------------------------------
//LSA Link Data Packet
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//LSU Packet Format
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSUHeader);

TypeId
LSUHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::ospf::LSUHeader")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
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

//-----------------------------------------------------------------------------
//LSR Packet Format
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSRHeader);

TypeId
LSRHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::ospf::LSRHeader")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
            .AddConstructor<LSRHeader> ()
    ;
    return tid;
}

LSRHeader::LSRHeader(std::vector<LSRPacket> LSRs)
        : m_LSRs (LSRs)
{
	//m_LSRPacketNum = LSRs.size();
}


TypeId
LSRHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSRHeader::GetSerializedSize () const
{
	uint32_t t = 0;
	for(auto lsrPacket : m_LSRs){
		t += lsrPacket.GetSerializedSize();
	}
	return t;
}

void
LSRHeader::Serialize (Buffer::Iterator i) const
{
	//i.WriteHtonU32(m_LSRPacketNum);
	for (uint32_t n = 0; n < m_LSRs.size(); n++)
    {
        m_LSRs[n].Serialize(i);
        i.Next(m_LSRs[n].GetSerializedSize());
    }

}

uint32_t
LSRHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    //m_LSRPacketNum = i.ReadNtohU32();
    LSRPacket lsrtemp = LSRPacket();
	NS_ASSERT(i.GetRemainingSize() % lsrtemp.GetSerializedSize() == 0);
	int d = i.GetRemainingSize();

	int lsrNum = (d) / lsrtemp.GetSerializedSize();

	for(int j = 0; j < lsrNum; j++){
		LSRPacket ls = LSRPacket();
		ls.Deserialize(i);
		m_LSRs.push_back(ls);
		i.Next(ls.GetSerializedSize());
	}


    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSRHeader::Print (std::ostream &os) const
{

}

bool
LSRHeader::operator== (LSRHeader const & o ) const
{
    return true;
}
//-----------------------------------------------------------------------------
//LSR Packet Foramt
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//LSR Packet Content
//-----------------------------------------------------------------------------
LSRPacket::LSRPacket(Ipv4Address LinkStateID, Ipv4Address AdvertisingRouter)
        : m_LinkStateID (LinkStateID),
          m_AdvertisingRouter (AdvertisingRouter)
{
    m_LinkStateType = 1;
}

NS_OBJECT_ENSURE_REGISTERED (LSRPacket);

TypeId
LSRPacket::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::ospf::LSRPacket")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
            .AddConstructor<LSRPacket> ()
    ;
    return tid;
}



TypeId
LSRPacket::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSRPacket::GetSerializedSize () const
{
    return 12;
}

void
LSRPacket::Serialize (Buffer::Iterator i) const
{
    i.WriteHtonU32 (m_LinkStateType);
    WriteTo (i, m_LinkStateID);
    WriteTo (i, m_AdvertisingRouter);

}

uint32_t
LSRPacket::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_LinkStateType = i.ReadNtohU32 ();
    ReadFrom (i, m_LinkStateID);
    ReadFrom (i, m_AdvertisingRouter);


    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSRPacket::Print (std::ostream &os) const
{

}

bool
LSRPacket::operator== (LSRPacket const & o ) const
{
    return true;
}
//-----------------------------------------------------------------------------
//LSR Packet Content
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//LSAack Packet Format
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (LSAackHeader);

TypeId
LSAackHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::ospf::LSAackHeader")
            .SetParent<Header> ()
            .SetGroupName ("OSPF")
            .AddConstructor<LSAackHeader> ()
    ;
    return tid;
}

LSAackHeader::LSAackHeader(std::vector<LSAHeader> LSAacks)
        : m_LSAacks (LSAacks)
{
	//m_ackNum = m_LSAacks.size();
}


TypeId
LSAackHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
LSAackHeader::GetSerializedSize () const
{
	uint32_t t = 0;
	for(uint32_t n = 0; n < m_LSAacks.size(); n++){
		t += m_LSAacks[n].GetSerializedSize();
	}
    return t;
}

void
LSAackHeader::Serialize (Buffer::Iterator i) const
{
	//i.WriteHtonU32 (m_ackNum);
    for(int n = 0; n < int(m_LSAacks.size()); n++)
    {
        m_LSAacks[n].Serialize(i);
        i.Next(m_LSAacks[n].GetSerializedSize());
    }
}

uint32_t
LSAackHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    //m_ackNum = i.ReadNtohU32 ();
    LSAHeader lsaHeadertemp = LSAHeader();
 	NS_ASSERT(i.GetRemainingSize() % lsaHeadertemp.GetSerializedSize() == 0);
 	int d = i.GetRemainingSize();

 	int lsaHeaderNum = (d) / lsaHeadertemp.GetSerializedSize();

 	for(int j = 0; j < lsaHeaderNum; j++){
    	LSAHeader ls = LSAHeader();
    	ls.Deserialize(i);
    	i.Next(ls.GetSerializedSize());
    	m_LSAacks.push_back(ls);
 	}


    uint32_t dist = i.GetDistanceFrom (start);
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
LSAackHeader::Print (std::ostream &os) const
{

}

bool
LSAackHeader::operator== (LSAackHeader const & o ) const
{
    return true;
}
//-----------------------------------------------------------------------------
//LSAack Packet Format
//-----------------------------------------------------------------------------
}
}
