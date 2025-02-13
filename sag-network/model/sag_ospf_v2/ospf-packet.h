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
#ifndef OSPFPACKET_H
#define OSPFPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>

#include "ns3/ospf-neighbor.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace ospf {

/**
* \ingroup ospf
* \brief MessageType enumeration
*/
enum MessageType
{
  OSPFTYPE_HELLO  = 1,   //!< OSPFTYPE_HELLO
  OSPFTYPE_DBD  = 2,   //!< OSPFTYPE_DBD
  OSPFTYPE_LSR  = 3,   //!< OSPFTYPE_LSR
  OSPFTYPE_LSU = 4, //!< OSPFTYPE_LSU
  OSPFTYPE_LSAck = 5 //!<OSPFTYPE_LSAck
};

/**
* \ingroup ospf
* \brief OSPF Packet types
*/
//class TypeHeader : public Header
//{
//public:
//  /**
//   * constructor
//   * \param t the OSPF HELLO type
//   */
//  TypeHeader (MessageType t = OSPFTYPE_HELLO);
//
//  /**
//   * \brief Get the type ID.
//   * \return the object TypeId
//   */
//  static TypeId GetTypeId ();
//  TypeId GetInstanceTypeId () const;
//  uint32_t GetSerializedSize () const;
//  void Serialize (Buffer::Iterator start) const;
//  uint32_t Deserialize (Buffer::Iterator start);
//  void Print (std::ostream &os) const;
//
//  /**
//   * \returns the type
//   */
//  MessageType Get () const
//  {
//    return m_type;
//  }
//  /**
//   * Check that type if valid
//   * \returns true if the type is valid
//   */
//  bool IsValid () const
//  {
//    return m_valid;
//  }
//  /**
//   * \brief Comparison operator
//   * \param o header to compare
//   * \return true if the headers are equal
//   */
//  bool operator== (TypeHeader const & o) const;
//private:
//  MessageType m_type; ///< type of the message
//  bool m_valid; ///< Indicates if the message is valid
//};
//
///**
//  * \brief Stream output operator
//  * \param os output stream
//  * \return updated stream
//  */
//std::ostream & operator<< (std::ostream & os, TypeHeader const & h);


/**
* \ingroup ospf
* \brief   OSPF Packet Header Format
  \verbatim
          0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |   Version #   |     Type      |         Packet length         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Router ID                            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Area ID                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           Checksum            |             AuType            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                       Authentication                          |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                       Authentication                          |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/

class OspfHeader : public Header
{
public:
  /**
   * constructor
   *
   * \param
   */
  OspfHeader(uint16_t packetLength = 0, MessageType type = OSPFTYPE_HELLO, Ipv4Address routerID = Ipv4Address (), Ipv4Address areaID = Ipv4Address (),
		     uint16_t checkSum = 0, uint16_t auType = 0, uint32_t authentication1 = 0, uint32_t authentication2 = 0);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  bool operator== (OspfHeader const & o) const;

  Ipv4Address GetRouterID (){
  	  return m_routerID;
  }

  Ipv4Address GetAreaID (){
	  return m_areaID;
  }

  uint16_t GetPacketLength (){
  	  return m_packetLength;
  }

  MessageType GetType(){
	  return m_type;
  }

  void SetOspfLength(uint16_t length){
	  m_packetLength = length;
  }




private:
  uint8_t        m_version;          ///< version 2
  MessageType		m_type;
  uint16_t        m_packetLength;       ///< Packet Length
  Ipv4Address       m_routerID;      ///< Router ID
  Ipv4Address       m_areaID;            ///< Area ID
  uint16_t        m_checkSum;       ///< CheckSum
  uint16_t        m_auType;         ///< AuType
  uint32_t       m_authentication1;    ///< Authentication
  uint32_t       m_authentication2;    ///< Authentication

};



/**
* \ingroup ospf
* \brief   Hello Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                        Network Mask                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         HelloInterval         |    Options    |    Rtr Pri    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     RouterDeadInterval                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      Designated Router                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   Backup Designated Router                    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Neighbor                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              ...                              |
  \endverbatim
*/


class HelloHeader : public Header
{
public:
  /**
   * constructor
   *
   * \param
   */
  HelloHeader(uint32_t mask = 0, Time helloInterval = MilliSeconds (0), uint8_t options = 0, uint8_t rtrPri = 1,
		  Time rtrDeadInterval = MilliSeconds (0), Ipv4Address dr = Ipv4Address(), Ipv4Address bdr = Ipv4Address(), std::vector<Ipv4Address> neighors = {});

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  bool operator== (HelloHeader const & o) const;

  Time GetHelloInterval (){
	  Time t (Seconds (m_helloInterval));
	  return t;
  }

  Time GetRouterDeadInterval (){
	  Time t (Seconds (m_rtrDeadInterval));
	  return t;
  }

  std::vector<Ipv4Address> GetNeighbors (){
	  return m_neighors;
  }

  uint8_t GetPriority (){
  	  return m_rtrPri;
  }


private:
  uint32_t       m_mask;          ///< Network Mask
  uint16_t        m_helloInterval;  ///< HelloInterval
  uint8_t        m_options;       ///< Options
  uint8_t        m_rtrPri;      ///< Router Priority
  uint32_t       m_rtrDeadInterval;  ///< RouterDeadInterval
  Ipv4Address       m_dr;       ///< Designated Router
  Ipv4Address       m_bdr;         ///< Backup Designated Router
  std::vector<Ipv4Address>       m_neighors;    ///< Neighbors
  //uint8_t           m_nbNum;



};

/**
* \ingroup ospf
* \brief   Database Description Packet Format
  \verbatim

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         Interface MTU         |    Options    |0|0|0|0|0|I|M|MS
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     DD sequence number                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               |
       +-                                                             -+
       |                                                               |
       +-                      An LSA Header                          -+
       |                                                               |
       +-                                                             -+
       |                                                               |
       +-                                                             -+
       |                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              ...                              |

  \endverbatim
*/

class DDHeader : public Header
{
public:
	/**
	* constructor
	*
	* \param
	*/
	DDHeader(uint16_t mtu = 0, uint8_t options = 0, uint8_t flags = 0, uint32_t seqNum = 0, std::vector<LSAHeader> lsaHeaders = {});

	/**
	* \brief Get the type ID.
	* \return the object TypeId
	*/
	static TypeId GetTypeId ();
	TypeId GetInstanceTypeId () const;
	uint32_t GetSerializedSize () const;
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	void Print (std::ostream &os) const;

	uint16_t GetMTU() {
		return m_mtu;
	}

	uint8_t GetFlags() {
		return m_flags;
	}

	uint32_t GetSeqNum() {
		return m_seqNum;
	}

	std::vector<LSAHeader> GetLSAHeaders(){
		return m_lsaHeaders;
	}

	void SetLSAHeaders(std::vector<LSAHeader> lsaHeaders){
		m_lsaHeaders = lsaHeaders;
	}


private:
	uint16_t m_mtu;     ///< Interface MTU
	uint8_t m_options;   ///< Options
	uint8_t m_flags;     ///< I M MS bit
	uint32_t m_seqNum;   ///< DD sequence number
	std::vector<LSAHeader> m_lsaHeaders;  ///< LSA headers
//	uint8_t m_lsNum;
};




}  // namespace ospf
}  // namespace ns3

#endif /* OSPFPACKET_H */
