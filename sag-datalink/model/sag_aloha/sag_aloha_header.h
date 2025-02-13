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

#ifndef SAG_ALOHA_HEADER_H
#define SAG_ALOHA_HEADER_H

#include "ns3/header.h"
#include "ns3/packet.h"
#include <ns3/mac48-address.h>

namespace ns3 {
namespace aloha {

/**
* \ingroup aodv
* \brief MessageType enumeration
*/
// we make a simplification
enum MessageType
{
  ALOHA_DATA  = 1,   //!< ALOHA_DATA
  ALOHA_ACK  = 2,   //!< ALOHA_ACK
};


/**
* \ingroup aodv
* \brief ALOHA Packet types
*/
class TypeHeader : public Header
{
public:
	/**
	* constructor
	* \param t the ALOHA type
	*/
	TypeHeader (MessageType t = ALOHA_DATA);

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

	/**
	* \returns the type
	*/
	MessageType Get () const
	{
	return m_type;
	}
	/**
	* Check that type if valid
	* \returns true if the type is valid
	*/
	bool IsValid () const
	{
	return m_valid;
	}
	/**
	* \brief Comparison operator
	* \param o header to compare
	* \return true if the headers are equal
	*/
	bool operator== (TypeHeader const & o) const;
private:
	MessageType m_type; ///< type of the message
	bool m_valid; ///< Indicates if the message is valid
};


class SAGAlohaHeader : public Header
{
public:

  /**
   * \brief Construct a aloha header.
   */
	SAGAlohaHeader ();

  /**
   * \brief Destroy a aloha header.
   */
  virtual ~SAGAlohaHeader ();

  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the TypeId of the instance
   *
   * \return The TypeId for this instance
   */
  virtual TypeId GetInstanceTypeId (void) const;


  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;


  void SetProtocol (uint16_t protocol);
  uint16_t GetProtocol (void);

  void SetSource(Mac48Address source);
  void SetDestination(Mac48Address destination);

  Mac48Address GetSource() const;
  Mac48Address GetDestination() const;

  void SetPacketSequence(uint64_t seq);
  uint64_t GetPacketSequence();

private:
  Mac48Address m_source;
  Mac48Address m_destination;

  /// The aloha protocol type of the payload packet
  uint16_t m_protocol;

  uint64_t m_packetSeq;
};

} // namespace aloha
} // namespace ns3


#endif /* PPP_HEADER_H */
