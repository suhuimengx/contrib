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
#ifndef SAG_ALOHA_CHANNEL_H
#define SAG_ALOHA_CHANNEL_H
#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/mac48-address.h"
#include "sag_aloha_net_device.h"
#include "ns3/sag_physical_layer_gsl.h"
namespace ns3 {
class Packet;
class SAGAlohaChannel : public SAGPhysicalLayerGSL
{
public:
	static TypeId GetTypeId (void);
	SAGAlohaChannel ();

	// Transmission
	virtual bool TransmitTo(
		  Ptr<const Packet> p,
		  Ptr<SAGLinkLayerGSL> srcNetDevice,
		  Ptr<SAGLinkLayerGSL> dstNetDevice,
		  Time txTime,
		  bool isSameSystem
	);

	/**
	 * \brief Maintain a Co-Channel Conflict Judgment Model
	 *
	 * When transmitting and propagating a data packet, if other data packet
	 * transmission events occur, it is recorded that all current data packets
	 * have co-channel conflicts.
	 *
	 * \param time The sum of transmission delay and propagation delay
	 * \param packet The packet pointer
	 * \param srcNetDevice Source netdevice
	 *
	 */
	void ConflictJudgment (Time time, Ptr<Packet> packet, Ptr<SAGLinkLayerGSL> srcNetDevice);

	/**
	 * \brief Receive event judgment
	 *
	 * Before triggering the data packet receiving event, it is first judged whether
	 * a co-channel conflict occurs, and if so, the data packet is discarded, otherwise
	 * the receiving event is triggered.
	 *
	 * \param packet The packet pointer
	 * \param srcNetDevice Source netdevice
	 * \param destNetDevice Destination netdevice
	 *
	 */
	void ReceivingJudgment (Ptr<Packet> packet, Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice);


private:

	/// Conflict time
	std::pair<Time, Time> m_channelConflictTime = std::make_pair(Seconds(0), Seconds(0));
	/// Conflict list
	std::map<Ptr<Packet>, bool> m_packetConflictList;
	/// Packet list
	std::map<Ptr<SAGLinkLayerGSL>, std::vector<Ptr<Packet>>> m_packetList;


};
} // namespace ns3
#endif /* SAG_ALOHA_CHANNEL_H */
