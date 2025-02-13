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

#ifndef SAG_CSMA_HEADER_H
#define SAG_CSMA_HEADER_H

#include "ns3/header.h"
#include "ns3/packet.h"
#include <ns3/mac48-address.h>

namespace ns3 {
namespace csma {


class SAGCsmaHeader : public Header
{
public:

  /**
   * \brief Construct a csma header.
   */
	SAGCsmaHeader ();

  /**
   * \brief Destroy a csma header.
   */
  virtual ~SAGCsmaHeader ();

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

  /// The csma protocol type of the payload packet
  uint16_t m_protocol;

  uint64_t m_packetSeq;
};

} // namespace csma
} // namespace ns3


#endif /* CSMA_HEADER_H */
