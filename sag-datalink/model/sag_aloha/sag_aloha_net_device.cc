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
#include "sag_aloha_net_device.h"
#include "ns3/sag_aloha_header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGAlohaNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SAGAlohaNetDevice);

TypeId 
SAGAlohaNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGAlohaNetDevice")
    .SetParent<SAGLinkLayerGSL> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGAlohaNetDevice> ()
	.AddAttribute ("ExpireTime",
				   "The time to wait for conflict detection",
				   TimeValue (MilliSeconds (4)),
				   MakeTimeAccessor (&SAGAlohaNetDevice::m_expireTime),
				   MakeTimeChecker ())
	.AddAttribute ("RetransmissionInterval",
				   "The time to wait for packet retransmission",
				   TimeValue (MilliSeconds (3)),
				   MakeTimeAccessor (&SAGAlohaNetDevice::m_retransmissionInterval),
				   MakeTimeChecker ())
	;
  return tid;
}


SAGAlohaNetDevice::SAGAlohaNetDevice ()
:SAGLinkLayerGSL()
{
  NS_LOG_FUNCTION (this);
}

SAGAlohaNetDevice::~SAGAlohaNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

bool
SAGAlohaNetDevice::TransmitStart (Ptr<Packet> p, const Address address){

	NS_LOG_FUNCTION (this << p);
	NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

	if(m_currentPkt == 0){
		NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
		m_currentPkt = p;
		m_txMachineState = BUSY;
	}
	else if(m_currentPkt == p){
//		if(this->GetNode()->GetId()==100)
//		std::cout<<this->GetNode()->GetId()<<std::endl;
		NS_ASSERT_MSG (m_txMachineState == BUSY, "Whenever reTransit, must be BUSY");
	}
	m_phyTxBeginTrace (m_currentPkt);


	Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
	Time txCompleteTime = txTime + m_tInterframeGap;         //transmit delay

	// for check ack or not
	aloha::TypeHeader typeHeader;
	p->PeekHeader(typeHeader);

	bool result = m_channel->TransmitStart (p, this, address, txTime);
	if (result == false)
	{
		// packet loss
		m_phyTxDropTrace (p);
		// arrange next packet
		NextPacketArrange();
	}
	else{
		TrackUtilization(true);
		// Separate uplink and downlink, special processing for broadcasting todo
		if(this->GetAddress() == this->GetChannel()->GetDevice(0)->GetAddress())
		{	// downlink, no conflict, no retransmit, no ack receive
			Simulator::Schedule (txCompleteTime, &SAGAlohaNetDevice::TransmitComplete, this, address);

		}
		else{
			// uplink
			// ack, no ack
			// Transmit and ReTransmitEvent will be given if the link exists
			if(typeHeader.Get() == aloha::MessageType::ALOHA_ACK){
		        throw std::runtime_error("No ack");
				Simulator::Schedule (txCompleteTime, &SAGAlohaNetDevice::TransmitComplete, this, address);
			}
			else{
				ArrangeReTransmit(address, txCompleteTime);
				//Simulator::Schedule (txCompleteTime, &SAGAlohaNetDevice::ArrangeReTransmit, this, address);

			}

		}

	}

	NS_LOG_FUNCTION (this << " done");
	return result;



}

void
SAGAlohaNetDevice::TransmitComplete (const Address destination){

	NS_LOG_FUNCTION (this);

	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");

	NS_ASSERT_MSG (m_currentPkt != 0, "SAGLinkLayerGSL::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace (m_currentPkt);

	TrackUtilization(false);
	NextPacketArrange ();

}

void
SAGAlohaNetDevice::TransmitCompleteR (){

	NS_LOG_FUNCTION (this);

	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");

	NS_ASSERT_MSG (m_currentPkt != 0, "SAGLinkLayerGSL::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace (m_currentPkt);

	TrackUtilization(false);

}

void
SAGAlohaNetDevice::ArrangeReTransmit (const Address destination, Time txCompleteTime){
	NS_LOG_FUNCTION (this);


	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if arranging retransmit");

	// Still busy until receiving ack
	//m_txMachineState = READY;

	NS_ASSERT_MSG (m_currentPkt != 0, "SAGLinkLayerGSL::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace (m_currentPkt);

	// arrange retransmission event
	m_retransmissionInterval = MilliSeconds((rand() % ( 30 - 2 + 1 )) + 2);   // [5, 30] ms  (rand() % (b - a +1)) + a

	NS_ASSERT_MSG (txCompleteTime < m_expireTime + m_retransmissionInterval, "Must be larger");
	m_completeEvent = Simulator::Schedule (txCompleteTime, &SAGAlohaNetDevice::TransmitCompleteR, this);


	m_reTransmitDestination = destination;
	m_retransmissionEvent = Simulator::Schedule (m_expireTime + m_retransmissionInterval, &SAGAlohaNetDevice::TransmitStart, this, m_currentPkt, destination);

//	if(this->GetNode()->GetId()==100)
//	std::cout<<Simulator::Now()<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;

}


void
SAGAlohaNetDevice::NextPacketArrange ()
{
	NS_LOG_FUNCTION (this);

	NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if arranging NextPacket");


	if(m_completeEvent.IsRunning()){
		m_completeEvent.Cancel();
		TransmitCompleteR();
	}
	m_txMachineState = READY;
	m_retransmissionEvent.Cancel();

	m_currentPkt = 0;

	Ptr<Packet> p = m_queue->Dequeue ();
	if (p == 0)
	{
	  NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
	  return;
	}
	Address next_dest = m_queueDests.front ();
	m_queueDests.pop ();

	//
	// Got another packet off of the queue, so start the transmit process again.
	//
	m_snifferTrace (p);
	m_promiscSnifferTrace (p);
	TransmitStart (p, next_dest);

}

void
SAGAlohaNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest)
{
	NS_LOG_FUNCTION (this << p << protocolNumber);

	aloha::SAGAlohaHeader alohaHeader;
	alohaHeader.SetProtocol(EtherToPpp (protocolNumber));
	alohaHeader.SetDestination(Mac48Address::ConvertFrom (dest));
	alohaHeader.SetSource(Mac48Address::ConvertFrom (this->GetAddress()));
	alohaHeader.SetPacketSequence(p->GetUid());
//	std::cout<<":::::::::::::::"<<this->GetNode()->GetId()<<std::endl;
//	std::cout<<":::::"<<p->GetUid()<<std::endl;
	p->AddHeader(alohaHeader);

	aloha::TypeHeader typeHeader(aloha::MessageType::ALOHA_DATA);
	p->AddHeader(typeHeader);

}

bool
SAGAlohaNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
	NS_LOG_FUNCTION (this << p << param);
	aloha::SAGAlohaHeader alohaHeader;
	aloha::TypeHeader typeHeader;
	p->RemoveHeader(typeHeader);
	p->RemoveHeader(alohaHeader);


	//*********************************************** todo, if not do this, arp request may be triggered
	//std::cout<<":::::"<<alohaHeader.GetPacketSequence()<<std::endl;
	bool found = false;
	for(uint32_t i = 0; i < this->GetChannel()->GetNDevices(); i++){
		if(alohaHeader.GetSource() == Mac48Address::ConvertFrom (this->GetChannel()->GetDevice(i)->GetAddress())){
			found = true;
			break;
		}

	}
	if(found == false){
		return false;
	}
	//***********************************************

	if(typeHeader.Get() == aloha::MessageType::ALOHA_DATA){

		// Separate uplink and downlink, special processing for broadcasting todo
		if(alohaHeader.GetSource() == Mac48Address::ConvertFrom (this->GetChannel()->GetDevice(0)->GetAddress())){
			// downlink, no ack
			param = PppToEther (alohaHeader.GetProtocol ());
			return true;
		}

		// uplink
		// send ack
		AckSend(alohaHeader.GetSource(), alohaHeader.GetPacketSequence());
		// protocol
		param = PppToEther (alohaHeader.GetProtocol ());
		return true;
	}
	else if(typeHeader.Get() == aloha::MessageType::ALOHA_ACK){
		// receive ack
		AckReceived(alohaHeader.GetSource(), alohaHeader.GetPacketSequence());
		return false;
	}
	else{
		return false;
	}

}

void
SAGAlohaNetDevice::AckSend (ns3::Mac48Address macDst, uint64_t packetId){

	Ptr<Packet> ack = Create<Packet>();

	aloha::SAGAlohaHeader alohaHeader;
	alohaHeader.SetProtocol(0);
	alohaHeader.SetDestination(macDst);
	alohaHeader.SetSource(Mac48Address::ConvertFrom (this->GetAddress()));
	alohaHeader.SetPacketSequence(packetId);
	ack->AddHeader(alohaHeader);

	aloha::TypeHeader typeHeader(aloha::MessageType::ALOHA_ACK);
	ack->AddHeader(typeHeader);

	if (IsLinkUp () == false)
	{
		m_macTxDropTrace (ack);
		return;
	}

	m_macTxTrace (ack);

//	if(this->GetNode()->GetId()==90){
//
//		std::cout<<Simulator::Now()<<">>>>>>>>>>>>>>>>>>"<<90<<std::endl;
//
//	}

	//
	// We should enqueue and dequeue the packet to hit the tracing hooks.
	//
	if (m_queue->Enqueue (ack))
	{
		m_queueDests.push ((Address)macDst);
		//
		// If the channel is ready for transition we send the packet right now
		//
		if (m_txMachineState == READY)
		{
		  ack = m_queue->Dequeue ();
		  Address next_dest = m_queueDests.front ();
		  m_queueDests.pop ();
		  m_snifferTrace (ack);
		  m_promiscSnifferTrace (ack);
		  TransmitStart (ack, next_dest);
		  return;
		}
		return;
	}

	// Enqueue may fail (overflow)
	m_macTxDropTrace (ack);


}

void
SAGAlohaNetDevice::AckReceived (ns3::Mac48Address macSrc, uint64_t packetId){

	if(m_currentPkt == 0){
		// iignore
		return;
	}
	if(m_currentPkt->GetUid() == packetId){
		// ack
//		if(this->GetNode()->GetId()==100){
//			std::cout<<Simulator::Now()<<"::::::::::::"<<100<<std::endl;
//		}
		NextPacketArrange ();
	}
	else{
		// ignore
	}


}

void
SAGAlohaNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  SAGLinkLayerGSL::DoDispose ();
}


} // namespace ns3

