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


#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ppp-header.h"
#include "ns3/node-container.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "ns3/ipv4.h"
#include "sag_csma_header.h"
#include "sag_csma_backoff.h"
#include "sag_csma_net_device.h"
#include "sag_csma_channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGCsmaNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SAGCsmaNetDevice);

TypeId 
SAGCsmaNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGCsmaNetDevice")
    .SetParent<SAGLinkLayerGSL> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGCsmaNetDevice> ()
	;
  return tid;
}


SAGCsmaNetDevice::SAGCsmaNetDevice ()
:SAGLinkLayerGSL()
{
  NS_LOG_FUNCTION (this);
}

SAGCsmaNetDevice::~SAGCsmaNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
SAGCsmaNetDevice::SetBackoffParams (Time slotTime, uint32_t minSlots, uint32_t maxSlots, uint32_t ceiling, uint32_t maxRetries)
{
  m_backoff.m_slotTime = slotTime;
  m_backoff.m_minSlots = minSlots;
  m_backoff.m_maxSlots = maxSlots;
  m_backoff.m_ceiling = ceiling;
  m_backoff.m_maxRetries = maxRetries;
}


bool
SAGCsmaNetDevice::TransmitStart(Ptr<Packet> p, const Address address)
{
	NS_LOG_FUNCTION(this << p);
	NS_LOG_LOGIC("UID is " << p->GetUid () << ")");

	// Separate uplink and downlink
	//uplink, idle detection
	if (this->GetAddress() != this->GetChannel()->GetDevice(0)->GetAddress()) {
		WireState ChannelState = m_channel->GetState();
		if (ChannelState != IDLE) {
			// The channel is busy -- backoff and rechedule TransmitStart() unless
			// we have exhausted all of our retries.
			if (m_backoff.MaxRetriesReached()) {
				//
				// Too many retries, abort transmission of packet
				//
				m_phyTxDropTrace (m_currentPkt);
				m_backoff.ResetBackoffTime();
				NextPacketArrange();
			}
			else {
				m_backoff.IncrNumRetries();
				Time backoffTime = m_backoff.GetBackoffTime();
				m_txMachineState = BUSY;

				NS_LOG_LOGIC("Channel busy, backing off for " << backoffTime.GetSeconds () << " sec");

				Simulator::Schedule(backoffTime, &SAGCsmaNetDevice::TransmitStart, this, p, address);
			}
			return true;
		}
		else {
			if (m_currentPkt == 0) {
				m_currentPkt = p;
				m_txMachineState = BUSY;
			}
			else if (m_currentPkt == p) {
				//	if(this->GetNode()->GetId()==100)
				//	std::cout<<this->GetNode()->GetId()<<std::endl;
				NS_ASSERT_MSG(m_txMachineState == BUSY, "Whenever reTransit, must be BUSY");
			}
			m_phyTxBeginTrace (m_currentPkt);

			Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
			Time txCompleteTime = txTime + m_tInterframeGap;    //transmit delay

			bool result = m_channel->TransmitStart(p, this, address, txTime);
			if (result == false) {
				// packet loss
				m_phyTxDropTrace(p);
				// arrange next packet
				NextPacketArrange();
			} else {
				TrackUtilization(true);
				Simulator::Schedule(txCompleteTime, &SAGCsmaNetDevice::TransmitComplete, this, address);
			}
			NS_LOG_FUNCTION(this << " done");
			return result;
		}
	}
	//downlink, no idle detection
	else {
		if (m_currentPkt == 0) {
			m_currentPkt = p;
			m_txMachineState = BUSY;
		}
		else if (m_currentPkt == p) {
			//	if(this->GetNode()->GetId()==100)
			//	std::cout<<this->GetNode()->GetId()<<std::endl;
			NS_ASSERT_MSG(m_txMachineState == BUSY, "Whenever reTransit, must be BUSY");
		}
		m_phyTxBeginTrace (m_currentPkt);

		Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
		Time txCompleteTime = txTime + m_tInterframeGap;        //transmit delay

		bool result = m_channel->TransmitStart(p, this, address, txTime);
		if (result == false) {
			// packet loss
			m_phyTxDropTrace(p);
			// arrange next packet
			NextPacketArrange();
		}
		else {
			TrackUtilization(true);
			Simulator::Schedule(txCompleteTime, &SAGCsmaNetDevice::TransmitComplete, this, address);
		}
		NS_LOG_FUNCTION(this << " done");
		return result;
	}
}


void
SAGCsmaNetDevice::TransmitComplete (const Address destination){

	NS_LOG_FUNCTION (this);

	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");

	NS_ASSERT_MSG (m_currentPkt != 0, "SAGLinkLayerGSL::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace (m_currentPkt);
	m_backoff.ResetBackoffTime();
	TrackUtilization(false);
	NextPacketArrange ();

}


void
SAGCsmaNetDevice::NextPacketArrange ()
{
	NS_LOG_FUNCTION (this);

	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if arranging NextPacket");

	m_txMachineState = READY;

	m_currentPkt = 0;

	Ptr<Packet> p = m_queue->Dequeue ();
	if (p == 0)
	{
	  NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
	  return;
	}
	Address next_dest = m_queueDests.front ();
	m_queueDests.pop ();

	m_snifferTrace (p);
	m_promiscSnifferTrace (p);
	TransmitStart (p, next_dest);

}

void
SAGCsmaNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest)
{
	NS_LOG_FUNCTION (this << p << protocolNumber);

	csma::SAGCsmaHeader csmaHeader;
	csmaHeader.SetProtocol(EtherToPpp (protocolNumber));
	csmaHeader.SetDestination(Mac48Address::ConvertFrom (dest));
	csmaHeader.SetSource(Mac48Address::ConvertFrom (this->GetAddress()));
	csmaHeader.SetPacketSequence(p->GetUid());
//	std::cout<<":::::::::::::::"<<this->GetNode()->GetId()<<std::endl;
//	std::cout<<":::::"<<p->GetUid()<<std::endl;
	p->AddHeader(csmaHeader);

}

bool
SAGCsmaNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
	NS_LOG_FUNCTION (this << p << param);
	csma::SAGCsmaHeader csmaHeader;
	p->RemoveHeader(csmaHeader);

	bool found = false;
	for(uint32_t i = 0; i < this->GetChannel()->GetNDevices(); i++){
		if(csmaHeader.GetSource() == Mac48Address::ConvertFrom (this->GetChannel()->GetDevice(i)->GetAddress())){
			found = true;
			break;
		}

	}
	if(found == false){
		return false;
	}

	// protocol
	param = PppToEther (csmaHeader.GetProtocol ());
	return true;

}



void
SAGCsmaNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  SAGLinkLayerGSL::DoDispose ();
}


} // namespace ns3

