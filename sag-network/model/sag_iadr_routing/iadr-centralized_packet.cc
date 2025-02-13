//
// Created by Yilei Wang on 2023/2/21.
//SAG_PLATFORM_MASTER_DR_CALCULATE_PATH_H

#include "iadr-centralized_packet.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"


namespace ns3 {
class Ipv4Address;
namespace iadr {


//-----------------------------------------------------------------------------
// Cenralized Packect Header
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (RTSHeader);

TypeId
RTSHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::iadr::RTSHeader")
            .SetParent<Header> ()
            .SetGroupName ("IADR")
            .AddConstructor<RTSHeader> ()
    ;
    return tid;
}

RTSHeader::RTSHeader(uint16_t Pathage , uint16_t PathLength , Ipv4Address local_routerID , Ipv4Address local_areaID )
        : m_Pathage (Pathage),
          m_PathLength (PathLength),
		  /*m_src_routerID (src_routerID),
          m_src_areaID (src_areaID),
          m_dst_routerID (dst_routerID),
          m_dst_areaID (dst_areaID),*/
		  m_local_routerID (local_routerID),
		  m_local_areaID (local_areaID)

{

}


TypeId
RTSHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
RTSHeader::GetSerializedSize () const
{
    return 12;
}

void
RTSHeader::Serialize (Buffer::Iterator i) const
{
    i.WriteHtonU16 (m_Pathage);
    i.WriteHtonU16 (m_PathLength);
    /*WriteTo (i, m_src_routerID);
    WriteTo (i, m_src_areaID);
    WriteTo (i, m_dst_routerID);
    WriteTo (i, m_dst_areaID);
    */
    WriteTo (i, m_local_routerID);
    WriteTo (i, m_local_areaID);
}

uint32_t
RTSHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_Pathage = i.ReadNtohU16 ();
    m_PathLength = i.ReadNtohU16();
    /*ReadFrom (i, m_src_routerID);
    ReadFrom (i, m_src_areaID);
    ReadFrom (i, m_dst_routerID);
    ReadFrom (i, m_dst_areaID);
    */
    ReadFrom (i, m_local_routerID);
    ReadFrom (i, m_local_areaID);


    uint32_t dist = i.GetDistanceFrom (start);
    //NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
RTSHeader::Print (std::ostream &os) const
{

}

bool
RTSHeader::operator== (RTSHeader const & o ) const
{
//	if(m_checksum == o.m_checksum)
//	{
//		if(m_LSsequence == o.m_LSsequence)
//		{
//			return true;
//		}
//	}
	if(m_local_routerID == o.m_local_routerID && m_local_areaID == o.m_local_areaID){
		return true;
	}
	else{
	return false;}
}
//-----------------------------------------------------------------------------
// CR Packet Header
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//CR Packet
//-----------------------------------------------------------------------------
NS_OBJECT_ENSURE_REGISTERED (RTSPacket);

TypeId
RTSPacket::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::iadr::RTSPacket")
            .SetParent<Header> ()
            .SetGroupName ("IADR")
            .AddConstructor<RTSPacket> ()
    ;
    return tid;
}

RTSPacket::RTSPacket(std::vector<Ipv4Address> srcip,std::vector<Ipv4Address> dst,std::vector<Ipv4Address> nexthop)
        :
		m_srcip (srcip),
		m_dst (dst),
		m_nexthop (nexthop)
{
	m_size=m_srcip.size();
}


TypeId
RTSPacket::GetInstanceTypeId () const
{
    return GetTypeId ();
}

uint32_t
RTSPacket::GetSerializedSize () const
{
    return uint32_t(m_size*12+4);
}

void
RTSPacket::Serialize (Buffer::Iterator i) const
{
	i.WriteHtonU32 (m_size);
	for (uint32_t it=0; it<m_size;it++)
	    {
		//std::cout<<"this is a test for Mengy!!"<<m_srcip[it].Get()<<std::endl;
			WriteTo (i, m_srcip[it]);
	    }
	for (uint32_t it=0; it<m_size;it++)
	    {
			WriteTo (i, m_dst[it]);
	    }
	for (uint32_t it=0; it<m_size;it++)
	    {
			WriteTo (i, m_nexthop[it]);
	    }

}

uint32_t
RTSPacket::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_size = i.ReadNtohU32 ();
    //std::cout<<  i.GetDistanceFrom (start) <<"uint32 size"<<std::endl;
    //ReadFrom (i, m_srcip[it]);
    //&m_srcip[0].Set (i.ReadNtohU32());
    //Ipv4Address test;
    m_srcip.resize(m_size);
    m_dst.resize(m_size);
    m_nexthop.resize(m_size);
    for (uint32_t it=0; it<m_size;it++)
    {
    	ReadFrom (i, m_srcip[it]);
    	//std::cout<<"this is a test for Mengy!!"<<m_srcip[it].Get()<<std::endl;
    }
    for (uint32_t it=0; it<m_size;it++)
    {
    	ReadFrom (i, m_dst[it]);
    }
    for (uint32_t it=0; it<m_size;it++)
    {
    	//std::cout<<  it <<"uint32 size"<<std::endl;
    	ReadFrom (i, m_nexthop[it]);
    }


    uint32_t dist = i.GetDistanceFrom (start);
    //std::cout<< dist <<"dist"<<std::endl;
    //std::cout<< GetSerializedSize () <<"GetSerializedSize"<<std::endl;
    NS_ASSERT (dist == GetSerializedSize ());
    return dist;
}

void
RTSPacket::Print (std::ostream &os) const
{

}

bool
RTSPacket::operator== (RTSPacket const & o ) const
{
			std::vector<Ipv4Address> llds = o.m_srcip;
            std::vector<Ipv4Address> llds2 = o.m_dst;
            std::vector<Ipv4Address> llds3 = o.m_nexthop;
			bool equalOrNot = true;
			for(uint32_t i= 0; i < llds.size(); i++){
				if(!(llds.at(i) == m_srcip.at(i) && llds2.at(i) == m_dst.at(i)&& llds3.at(i)==m_nexthop.at(i))){
					equalOrNot = false;
					break;
				}
			}
			return equalOrNot;
}



//-----------------------------------------------------------------------------
//CR Packet
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//CRP Header Format
//-----------------------------------------------------------------------------
        NS_OBJECT_ENSURE_REGISTERED (RTSPHeader);

        TypeId
        RTSPHeader::GetTypeId ()
        {
            static TypeId tid = TypeId ("ns3::iadr::RTSPHeader")
                    .SetParent<Header> ()
                    .SetGroupName ("IADR")
                    .AddConstructor<RTSPHeader> ()
            ;
            return tid;
        }

        RTSPHeader::RTSPHeader(std::pair<RTSHeader, RTSPacket> RTSs)
                : m_RTSs (RTSs)
        {

        }


        TypeId
        RTSPHeader::GetInstanceTypeId () const
        {
            return GetTypeId ();
        }

        uint32_t
        RTSPHeader::GetSerializedSize () const
        {
            uint32_t t=m_RTSs.second.GetSize();

            return (12 + t*12+ 4);
        }

        void
        RTSPHeader::Serialize (Buffer::Iterator i) const
        {

            m_RTSs.first.Serialize(i);
            i.Next(m_RTSs.first.GetSerializedSize ());
            m_RTSs.second.Serialize(i);
            i.Next(m_RTSs.second.GetSerializedSize ());
            /*
            i.WriteHtonU16 (m_RTSs.first.GetPathage());
            WriteTo (i, m_RTSs.first.GetLocal_RouterID());
            WriteTo (i, m_RTSs.first.GetLocal_AreaID());
            for(uint32_t n = 0; n < m_RTSs.first.GetPathLength(); n++)
            {
                std::vector<Ipv4Address> m_r=m_RTSs.second.GetRTSPathRouters();
                WriteTo(i,m_r[n]);
            }
            for(uint32_t n = 0; n < m_RTSs.first.GetPathLength(); n++)
            {
                std::vector<Ipv4Address> m_a=m_RTSs.second.GetRTSPathAreas();
                WriteTo(i,m_a[n]);
            }
            */
        }

        uint32_t
        RTSPHeader::Deserialize (Buffer::Iterator start)
        {
            Buffer::Iterator i = start;

            RTSHeader rtsHeader=RTSHeader();
            rtsHeader.Deserialize (i);
            //
            i.Next(rtsHeader.GetSerializedSize());
            m_RTSs.first=rtsHeader;
            RTSPacket rtsPacket=RTSPacket();
            rtsPacket.Deserialize (i);
            //
            i.Next(rtsPacket.GetSerializedSize());
            m_RTSs.second=rtsPacket;
            //m_RTSs=std::make_pair(rtsHeader, rtsPacket);
            /*m_RTSs.first.GetPathage() = i.ReadNtohU16 ();
            m_RTSs.first.GetPathLength = i.ReadNtohU16();
            ReadFrom (i, m_RTSs.first.GetSrc_RouterID());
            ReadFrom (i, m_RTSs.first.GetSrc_AreaID());
            ReadFrom (i, m_RTSs.first.GetDst_RouterID());
            ReadFrom (i, m_RTSs.first.GetDst_AreaID());

            for(uint32_t n = 0; n < m_RTSs.first.GetPathLength(); n++)
            {
;                std::vector<Ipv4Address> m_r=m_RTSs.second.GetRTSPathRouters();
                ReadFrom (i, m_RTSs.second.m_r[n]);
            }

            for(uint32_t n = 0; n < m_RTSs.first.GetPathLength(); n++)
            {
                std::vector<Ipv4Address> m_a=m_RTSs.second.GetRTSPathAreas();
                ReadFrom (i,m_RTSs.second.m_a[n]);
            }*/

            uint32_t dist = i.GetDistanceFrom (start);
            NS_ASSERT (dist == GetSerializedSize ());
            return dist;
        }

        void
        RTSPHeader::Print (std::ostream &os) const
        {

        }

        bool
        RTSPHeader::operator== (RTSPHeader const & o ) const
        {
            return true;
        }



}
}
