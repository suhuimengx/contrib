/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 * (Based on point-to-point channel)
 * Author: Xu    June 2022
 * 
 */


#ifndef SAG_PHYSICA_LAYER_H
#define SAG_PHYSICA_LAYER_H

#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/sag_link_layer.h"
#include "ns3/channel.h"


namespace ns3 {

class SAGLinkLayer;
class Packet;

/**
 * \brief SAGPhysicalLayer
 * 
 * Channel connecting two satellites 
 *
 * This class represents a very simple point to point channel.  There is no
 * multi-drop capability on this channel -- there can be a maximum of two 
 * point-to-point net devices connected.
 *
 * There are two "wires" in the channel.  The first device connected gets the
 * [0] wire to transmit on.  The second device gets the [1] wire.  There is a
 * state (IDLE, TRANSMITTING) associated with each wire.
 * 
 *
 */
class SAGPhysicalLayer : public Channel
{

public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Create a PointToPointLaserChannel
   *
   */
  SAGPhysicalLayer ();

  /**
   * \brief Attach a given netdevice to this channel
   * 
   * \param device pointer to the netdevice to attach to the channel
   */
  virtual void Attach (Ptr<SAGLinkLayer> device);

  /**
   * \brief Transmit a packet over this channel
   * 
   * \param p Packet to transmit
   * \param src source PointToPointLaserNetDevice
   * \param node_other_end node at the other end of the channel
   * \param txTime transmission time
   * \returns true if successful (always true)
   */
  virtual bool TransmitStart (Ptr<const Packet> p, Ptr<SAGLinkLayer> src, Ptr<Node> node_other_end, Time txTime);

  /**
   * \brief Write the traffic sent to each node (link utilization) to a stringstream
   * 
   * \param str the string stream
   * \param node_id the source node of the traffic
   */
  //virtual void WriteTraffic(std::stringstream& str, uint32_t node_id);

  /**
   * \brief Get number of devices on this channel
   * 
   * \returns number of devices on this channel
   */
  virtual std::size_t GetNDevices (void) const;

  /**
   * \brief Get PointToPointLaserNetDevice corresponding to index i on this channel
   *
   * \param i Index number of the device requested
   *
   * \returns Ptr to PointToPointLaserNetDevice requested
   */
  virtual Ptr<SAGLinkLayer> GetPointToPointLaserDevice (std::size_t i) const;

  /**
   * \brief Get NetDevice corresponding to index i on this channel
   *
   * \param i Index number of the device requested
   *
   * \returns Ptr to NetDevice requested
   */
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

  virtual void SetChannelDelay(Time delay);

  void RouteHopCountTrace(uint32_t nodeid1, uint32_t nodeid2, Ptr<Packet> p);


}; // namespace ns3

}

#endif /* SAG_PHYSICA_LAYER_H */
