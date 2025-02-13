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


#ifndef SAG_ALOHA_NET_DEVICE_H
#define SAG_ALOHA_NET_DEVICE_H

#include <cstring>
#include <queue>
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include "ns3/node-container.h"
#include "ns3/sag_link_layer_gsl.h"

namespace ns3 {



class SAGAlohaNetDevice: public SAGLinkLayerGSL
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a SAGLinkLayerGSL
   *
   * This is the constructor for the SAGLinkLayerGSL
   */
  SAGAlohaNetDevice ();

  /**
   * Destroy a SAGLinkLayerGSL
   *
   * This is the destructor for the SAGLinkLayerGSL.
   */
  virtual ~SAGAlohaNetDevice ();



private:

  /**
   * \brief Assign operator
   *
   * The method is private, so it is DISABLED.
   *
   * \param o Other NetDevice
   * \return New instance of the NetDevice
   */
  SAGAlohaNetDevice& operator = (const SAGAlohaNetDevice &o);

  /**
   * \brief Copy constructor
   *
   * The method is private, so it is DISABLED.

   * \param o Other NetDevice
   */
  SAGAlohaNetDevice (const SAGAlohaNetDevice &o);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

protected:

  bool TransmitStart (Ptr<Packet> p, const Address address);
  void TransmitComplete (const Address destination);
  void TransmitCompleteR ();

  /**
   * \brief Arrange packet retransmit event
   *
   * \param destination Destination mac address
   *
   */
  void ArrangeReTransmit (const Address destination,  Time txCompleteTime);
  /**
   * \brief Arrange next packet transmit
   *
   */
  void NextPacketArrange ();
  /**
   * Adds the necessary headers and trailers to a packet of data in order to
   * respect the protocol implemented by the agent.
   * \param p packet
   * \param protocolNumber protocol number
   */
  virtual void AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest);

  /**
   * Removes, from a packet of data, all headers and trailers that
   * relate to the protocol implemented by the agent
   * \param p Packet whose headers need to be processed
   * \param param An integer parameter that can be set by the function
   * \return Returns true if the packet should be forwarded up the
   * protocol stack.
   */
  virtual bool ProcessHeader (Ptr<Packet> p, uint16_t& param);

  /**
   * \brief Send ack packet
   *
   * \param macDst Destination mac address
   * \param packetId Packet uid
   *
   */
  void AckSend (ns3::Mac48Address macDst, uint64_t packetId);

  /**
   * \brief Receive ack packet
   *
   * \param macSrc Source mac address
   * \param packetId Packet uid
   *
   */
  void AckReceived (ns3::Mac48Address macSrc, uint64_t packetId);


private:

  /// packet retransmission event
  EventId m_retransmissionEvent;

  /// retransmission destination
  Address m_reTransmitDestination;

  /// packet retransmission interval
  Time m_retransmissionInterval;
  EventId m_completeEvent;

  /// expire time
  Time m_expireTime;

};

} // namespace ns3

#endif /* SAG_ALOHA_NET_DEVICE_H */

