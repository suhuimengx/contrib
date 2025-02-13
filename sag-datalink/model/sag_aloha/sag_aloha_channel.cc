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
 */


#include "ns3/core-module.h"
#include "ns3/abort.h"
//#include "ns3/mpi-interface.h"
#include <vector>
#include "sag_aloha_channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGAlohaChannel");

NS_OBJECT_ENSURE_REGISTERED (SAGAlohaChannel);

TypeId 
SAGAlohaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGAlohaChannel")
    .SetParent<SAGPhysicalLayerGSL> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGAlohaChannel> ()
  ;
  return tid;
}

SAGAlohaChannel::SAGAlohaChannel()
  :
	SAGPhysicalLayerGSL ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


bool
SAGAlohaChannel::TransmitTo(Ptr<const Packet> p, Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice, Time txTime, bool isSameSystem) {

  // Mobility models for source and destination
  Ptr<MobilityModel> senderMobility = srcNetDevice->GetNode()->GetObject<MobilityModel>();
  Ptr<Node> receiverNode = destNetDevice->GetNode();
  Ptr<MobilityModel> receiverMobility = receiverNode->GetObject<MobilityModel>();

  Ptr<Packet> pktCopy = p->Copy ();
  RouteHopCountTrace(srcNetDevice->GetNode()->GetId(), destNetDevice->GetNode()->GetId(), pktCopy);

  // Calculate delay
  Time delay = this->GetDelay(senderMobility, receiverMobility);
  NS_LOG_DEBUG(
          "Sending packet " << p << " from node " << srcNetDevice->GetNode()->GetId()
          << " to " << destNetDevice->GetNode()->GetId() << " with delay " << delay
  );

  // Distributed mode is not enabled
  NS_ABORT_MSG_UNLESS(isSameSystem, "MPI distributed mode is currently not supported by the ALOHA GSL channel.");

  // Schedule arrival of packet at destination network device
  if(srcNetDevice == m_sat_net_device){
	  // Downlink: no conflict
	  Simulator::ScheduleWithContext(
				receiverNode->GetId(),
				txTime + delay,
				&SAGAlohaNetDevice::Receive,
				destNetDevice,
				pktCopy
	  );
  }
  else{

	  // Uplink: maybe conflict
	  Simulator::Schedule(txTime + delay,
	  					&SAGAlohaChannel::ReceivingJudgment,
						this,
						pktCopy,
						srcNetDevice,
						destNetDevice
	  );
	  ConflictJudgment(txTime + delay, pktCopy, srcNetDevice);


  }


  return true;
}

void
SAGAlohaChannel::ConflictJudgment (Time time, Ptr<Packet> packet, Ptr<SAGLinkLayerGSL> srcNetDevice){

	if(m_channelConflictTime.first == m_channelConflictTime.second){

		m_channelConflictTime.first = Simulator::Now();
		m_channelConflictTime.second = Simulator::Now() + time;

		// Record packet list
		m_packetList[srcNetDevice].push_back(packet);
		// Record packet conflict list
		m_packetConflictList[packet] = false;

	}
	else{
		NS_ASSERT_MSG(m_channelConflictTime.second >= Simulator::Now(), "ChannelConflictTime has not been updated");
		NS_ASSERT_MSG(m_channelConflictTime.first <= Simulator::Now(), "ChannelConflictTime has not been updated");
		NS_ASSERT_MSG(m_packetConflictList.size() != 0, "ChannelConflictTime has not been updated");

		auto pIter = m_packetList.find(srcNetDevice);
		if(pIter != m_packetList.end() && m_packetList.size() == 1){
			NS_ASSERT_MSG(m_packetList[srcNetDevice].size() != 0, "PacketList has not been updated");
			// no conflict
			// Record packet list
			m_packetList[srcNetDevice].push_back(packet);
			// Record packet conflict list
			m_packetConflictList[packet] = false;

			if(m_channelConflictTime.second < Simulator::Now() + time){
				m_channelConflictTime.second = Simulator::Now() + time;
			}
			m_channelConflictTime.first = Simulator::Now();


			return;
		}


		// conflict occur
		for(auto entry : m_packetConflictList){
			m_packetConflictList[entry.first] = true;
		}
		m_packetConflictList[packet] = true;
		m_packetList[srcNetDevice].push_back(packet);

		if(m_channelConflictTime.second < Simulator::Now() + time){
			m_channelConflictTime.second = Simulator::Now() + time;
		}
		m_channelConflictTime.first = Simulator::Now();



	}


}

void
SAGAlohaChannel::ReceivingJudgment (Ptr<Packet> packet, Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice){

	// update packet conflict list
	auto iter = m_packetConflictList.find(packet);
	NS_ASSERT_MSG(iter != m_packetConflictList.end(), "Logic Wrong");
	if(iter->second){
		// packet loss, some trace? todo
		//std::cout<<srcNetDevice->GetNode()->GetId()<<"  "<<packet->GetUid()<<std::endl;
	}
	else{
		// the other end of the link receiving
		Simulator::ScheduleWithContext(
				destNetDevice->GetNode()->GetId(),
				Time(0),
				&SAGAlohaNetDevice::Receive,
				destNetDevice,
				packet
		);
//		if(srcNetDevice->GetNode()->GetId() >= 100){
//			std::cout<<Simulator::Now()<<"    "<<srcNetDevice->GetNode()->GetId()<<std::endl;
//		}

	}
	m_packetConflictList.erase(iter);

	// update packet list
	auto pIter = find(m_packetList[srcNetDevice].begin(), m_packetList[srcNetDevice].end(), packet);
	NS_ASSERT_MSG(pIter != m_packetList[srcNetDevice].end(), "Logic Wrong");
	m_packetList[srcNetDevice].erase(pIter);
	if(m_packetList[srcNetDevice].size() == 0){
		m_packetList.erase(srcNetDevice);
	}

	if(Simulator::Now() == m_channelConflictTime.second && m_packetConflictList.size() == 0){
		m_channelConflictTime = std::make_pair(Seconds(0), Seconds(0));
	}


}

} // namespace ns3
