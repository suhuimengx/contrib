/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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
 * (Based on point-to-point-laser network device)
 * Author: Xu    June 2022
 * 
 */

#ifndef SAG_LINK_LAYER_H
#define SAG_LINK_LAYER_H

#include <cstring>
#include <ns3/address.h>
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"


namespace ns3
{
    
template <typename Item> class Queue;
class ErrorModel;
class SAGPhysicalLayer;


/**
 * Enumeration of the states of the P2P link of the net device.
 */
enum P2PLinkState
{
	NORMAL,  /**< The transmitter is normal operation to transmit or receive */
	DISABLE, /**< The transmitter is disable to transmit or receive */
	ESTABLISH,
	TRANSMIT,
	TERMINATE,
};

/**
 * Enumeration of the interruption type of the ISL.
 */
enum P2PInterruptionType
{
	Unpredictable,  /**< Unpredictable ISL outages, such as those brought about by radiation */
	Predictable, /**< Predictable ISL outages, such as periodic closures of inter-orbit links */
};

class SAGLinkLayer : public NetDevice
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a SAGLinkLayer
   *
   * This is the constructor for the SAGLinkLayer
   */
  SAGLinkLayer ();

  /**
   * Destroy a SAGLinkLayer
   *
   * This is the destructor for the SAGLinkLayer.
   */
  virtual ~SAGLinkLayer ();


  /**
   * Set the Data Rate used for transmission of packets.  The data rate is
   * set in the Attach () method from the corresponding field in the channel
   * to which the device is attached.  It can be overridden using this method.
   *
   * \param bps the data rate at which this object operates
   */
  virtual void SetDataRate (DataRate bps);

  virtual uint64_t GetDataRate ();

  /**
   * Set the interframe gap used to separate packets.  The interframe gap
   * defines the minimum space required between packets sent by this device.
   *
   * \param t the interframe gap time
   */
  virtual void SetInterframeGap (Time t);

  /**
   * Attach the device to a channel.
   *
   * \param ch Ptr to the channel to which this object is being attached.
   * \return true if the operation was successful (always true actually)
   */
  virtual bool Attach (Ptr<SAGPhysicalLayer> ch);

  /**
   * Attach a queue to the PointToPointLaserNetDevice.
   *
   * The PointToPointLaserNetDevice "owns" a queue that implements a queueing
   * method such as DropTailQueue or RedQueue
   *
   * \param queue Ptr to the new queue.
   */
  virtual void SetQueue (Ptr<Queue<Packet> > queue);

  /**
   * Get a copy of the attached Queue.
   *
   * \returns Ptr to the queue.
   */
  virtual Ptr<Queue<Packet> > GetQueue (void) const;

  /**
   * Attach a receive ErrorModel to the PointToPointLaserNetDevice.
   *
   * The PointToPointNetLaserDevice may optionally include an ErrorModel in
   * the packet receive chain.
   *
   * \param em Ptr to the ErrorModel.
   */
  virtual void SetReceiveErrorModel (Ptr<ErrorModel> em);

  /**
   * Receive a packet from a connected PointToPointLaserChannel.
   *
   * The PointToPointLaserNetDevice receives packets from its connected channel
   * and forwards them up the protocol stack.  This is the public method
   * used by the channel to indicate that the last bit of a packet has
   * arrived at the device.
   *
   * \param p Ptr to the received packet.
   */
  virtual void Receive (Ptr<Packet> p);

  virtual void SetLinkState (P2PLinkState p2pLinkState);


  // The remaining methods are documented in ns3::NetDevice*
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;

  virtual void SetDestinationNode (Ptr<Node> node);
  virtual Ptr<Node> GetDestinationNode (void) const;

  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;

  virtual bool IsLinkUp (void) const;

  virtual void AddLinkChangeCallback (Callback<void> callback);

  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;

  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;

  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual double GetQueueOccupancyRate();
  virtual uint32_t GetMaxsize();

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

  virtual void SetInterruptionInformation(P2PInterruptionType metaData, bool b);
  virtual bool GetInterruptionInformation(P2PInterruptionType metaData);



public:
    void EnableUtilizationTracking(int64_t interval_ns);
    const std::vector<double>& FinalizeUtilization();

protected:
    void TrackUtilization(bool next_state_is_on);

private:
  bool m_utilization_tracking_enabled = false;
  int64_t m_interval_ns;
  int64_t m_prev_time_ns;
  int64_t m_current_interval_start;
  int64_t m_current_interval_end;
  int64_t m_idle_time_counter_ns;
  int64_t m_busy_time_counter_ns;
  bool m_current_state_is_on;
  std::vector<double> m_utilization;

};













} // namespace ns3

#endif /* SAG_LINK_LAYER_H */
