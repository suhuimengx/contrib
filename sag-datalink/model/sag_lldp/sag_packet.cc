/*
 * sag_packet.cc
 *
 *  Created on: 2023年10月14日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_CC_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_CC_

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "sag_packet.h"
#include "sag_tlv_struct.h"

namespace ns3 {

//*
//***********************LLDP Header******************************//
LLDPHeader::LLDPHeader()
{
	uint8_t dst[6] = {1,2,3,4,5,6};
	uint8_t src[6] = {1,2,3,4,5,6};
	uint16_t ethertype = 0x88cc;
	memcpy(m_dst, dst, sizeof(m_dst));
	memcpy(m_src, src, sizeof(m_src));
	m_ethertype = ethertype;
}

LLDPHeader::LLDPHeader(uint8_t dst[6] , uint8_t src[6], uint16_t ethertype)
{
	ethertype = 0x88cc;
	memcpy(m_dst, dst, sizeof(m_dst));
	memcpy(m_src, src, sizeof(m_src));
	m_ethertype = ethertype;
}

NS_OBJECT_ENSURE_REGISTERED (LLDPHeader);

TypeId
LLDPHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::LLDPHeader")
    .SetParent<Header> ()
    .SetGroupName ("LLDP")
    .AddConstructor<LLDPHeader> ()
  ;
  return tid;
}


TypeId
LLDPHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LLDPHeader::GetSerializedSize () const
{
  return 14;
}

void
LLDPHeader::Serialize (Buffer::Iterator i) const
{
	for(int a=0; a< 6; a++)
	{
		i.WriteU8(m_dst[a]);
	}

	for(int a=0; a< 6; a++)
	{
		i.WriteU8(m_src[a]);
	}

	i.WriteHtonU16 (m_ethertype);

}

uint32_t
LLDPHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  //std::cout<<"Receive a LLDPHeader Packet! Deserialize it!"<<std::endl;

/*  m_dst[0] = 0x01;
  m_dst[1] = 0x80;*/
  for(int a =0; a<6;a++)
  {
	  m_dst[a]=i.ReadU8();
  }

  for(int a =0; a<6;a++)
  {
	  m_src[a] = i.ReadU8();
  }

  m_ethertype = i.ReadNtohU16();

  //td::cout<<"Deserialize LLDP Header Done!"<<std::endl;


  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

uint8_t*
LLDPHeader::GetSrcMacAddress()
{
	return m_src;
}

uint8_t*
LLDPHeader::GetDesMacAddress()
{
	return m_dst;
}

uint16_t
LLDPHeader::GetEtherType()
{
	return m_ethertype;
}

void
LLDPHeader::Print (std::ostream &os) const
{

}

//*
//***********************LLDP PDU******************************//
LLDPPDU::LLDPPDU()
{
	m_tlv_list = new struct lldp_tlv_list;
}

LLDPPDU::LLDPPDU(struct lldp_tlv_list* tlv_list)
{
	m_tlv_list = tlv_list;
}

NS_OBJECT_ENSURE_REGISTERED (LLDPPDU);

TypeId
LLDPPDU::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::LLDPPDU")
    .SetParent<Header> ()
    .SetGroupName ("LLDP")
    .AddConstructor<LLDPPDU> ()
  ;
  return tid;
}


TypeId
LLDPPDU::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LLDPPDU::GetSerializedSize () const
{
	uint32_t size = 0;
	struct lldp_tlv_list* tmp = m_tlv_list;
	while(tmp != NULL) {
		size = size + 2 + tmp->tlv->length;
		tmp = tmp->next;
	}
  return size;
}

void
LLDPPDU::Serialize (Buffer::Iterator i) const
{
	struct lldp_tlv_list* tmp = m_tlv_list;
	while(tmp != NULL) {
		struct lldp_tlv* tlv = tmp ->tlv;
		uint16_t type_and_length;
		type_and_length = tlv->type;
		type_and_length = type_and_length << 9;
		type_and_length |= tlv->length;

		i.WriteHtonU16(type_and_length);
		for(uint16_t a = 0; a<tmp->tlv->length;a++)
		{
			i.WriteU8(tmp->tlv->info_string[a]);
		}
		tmp = tmp->next;
	}
}

uint32_t
LLDPPDU::Deserialize (Buffer::Iterator start)
{
	//std::cout<<"Receive a LLDPPDU Packet! Deserialize it!"<<std::endl;

	Buffer::Iterator i = start;
	//std::cout<<"Start Deserialize!"<<std::endl;

	struct lldp_tlv* tlv = new lldp_tlv;
	uint16_t type_and_length = i.ReadNtohU16();

    tlv->length = type_and_length & 0x01FF;
    tlv->type = type_and_length >> 9;

	uint8_t temp[tlv->length];
	for(uint16_t a = 0; a<tlv->length;a++)
	{
		temp[a] = i.ReadU8();
	}
	tlv->info_string = temp;

    m_tlv_list->tlv = tlv;

    struct lldp_tlv_list* first_tlv_list = m_tlv_list;

	while(m_tlv_list->tlv->type!=0)
	{
		struct lldp_tlv_list* tmp_list = new lldp_tlv_list;
		struct lldp_tlv* tmp_tlv = new lldp_tlv;

		uint16_t type_and_length_temp = i.ReadNtohU16();
		tmp_tlv->length = type_and_length_temp & 0x01FF;
		tmp_tlv->type = type_and_length_temp >> 9;

		uint8_t temp_array[tmp_tlv->length];
		for(uint16_t a = 0; a<tmp_tlv->length;a++)
		{
			temp_array[a] = i.ReadU8();
		}

		tmp_tlv->info_string = new uint8_t[tmp_tlv->length];
		memcpy(tmp_tlv->info_string,temp_array,tmp_tlv->length);


		tmp_list -> tlv = tmp_tlv;
		m_tlv_list->next = tmp_list;

		m_tlv_list = tmp_list;
	}
	m_tlv_list = first_tlv_list;

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());

  //std::cout<<"Deserialize Done!"<<std::endl;
  return dist;
}


 struct lldp_tlv_list*
 LLDPPDU::GetTLVList()
 {
	 return m_tlv_list;
 }

 void
 LLDPPDU::Print (std::ostream &os) const
 {

 }


};



#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_CC_ */
