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
#ifndef SAG_CSMA_CHANNEL_H
#define SAG_CSMA_CHANNEL_H
#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/mac48-address.h"
#include "sag_csma_net_device.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "sag_csma_channel.h"
#include <map>

namespace ns3 {

class Packet;

class SAGCsmaChannel : public SAGPhysicalLayerGSL
{
public:
	static TypeId GetTypeId (void);
	SAGCsmaChannel ();


	virtual bool TransmitStart (
		  Ptr<const Packet> p,
		  Ptr<SAGLinkLayerGSL> src,
		  Address dst_address,
		  Time txTime);

	// Transmission
	virtual bool TransmitTo(
		  Ptr<const Packet> p,
		  Ptr<SAGLinkLayerGSL> srcNetDevice,
		  Ptr<SAGLinkLayerGSL> dstNetDevice,
		  Time txTime,
		  bool isSameSystem
	);


	/**
	 * \brief Receive event judgment
	 *
	 * Before triggering the data packet receiving event, it is first judged whether
	 * a co-channel conflict occurs, and if so, the data packet is discarded, otherwise
	 * the receiving event is triggered.                                                      //ZHIJIE receive and GAIBIAN state
	 *
	 * \param packet The packet pointer
	 * \param srcNetDevice Source netdevice
	 * \param destNetDevice Destination netdevice
	 *
	 */
	void PacketReceiving (Ptr<Packet> packet, Ptr<SAGLinkLayerGSL> destNetDevice);





};

} // namespace ns3
#endif /* SAG_CSMA_CHANNEL_H */
