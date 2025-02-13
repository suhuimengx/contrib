/*
 * sag_packet.h
 *
 *  Created on: 2023年10月14日
 *      Author: kolimn
 */

#ifndef CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_H_
#define CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_H_

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"

#include "ns3/nstime.h"
#include "sag_tlv_struct.h"

namespace ns3 {
class LLDPHeader : public Header
{
public:

	/**
	 * Construct a LLDP Header
	 *
	 * \param dsc, the destination MAC address
	 * \param src, the source MAC address
	 * \param ethertype, Ethernet Type
	 *
	 */
	LLDPHeader(uint8_t dsc[6], uint8_t src[6], uint16_t ethertype);
	LLDPHeader();



  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  uint8_t* GetSrcMacAddress();
  uint8_t* GetDesMacAddress();
  uint16_t GetEtherType();

  void Print (std::ostream &os) const;




private:
  uint8_t m_dst[6];
  uint8_t m_src[6];
  uint16_t m_ethertype;
};

class LLDPPDU : public Header
{
public:

	/**
	 * Construct a LLDP PDU
	 *
	 * \param tlv_list, the tlv list to write in this PDU
	 *
	 */
	LLDPPDU(struct lldp_tlv_list* tlv_list);
	LLDPPDU();


  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  struct lldp_tlv_list* GetTLVList();

  void Print (std::ostream &os) const;




private:
  struct lldp_tlv_list* m_tlv_list;
};




}


#endif /* CONTRIB_PROTOCOLS_MODEL_SAG_LLDP_SAG_PACKET_H_ */
