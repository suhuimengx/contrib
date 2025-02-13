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
#include "sag_link_layer_gsl.h"
#include "ns3/sag_physical_layer_gsl.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAGLinkLayerGSL");

NS_OBJECT_ENSURE_REGISTERED (SAGLinkLayerGSL);

TypeId 
SAGLinkLayerGSL::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAGLinkLayerGSL")
    .SetParent<NetDevice> ()
    .SetGroupName ("GSL")
    .AddConstructor<SAGLinkLayerGSL> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&SAGLinkLayerGSL::SetMtu,
                                         &SAGLinkLayerGSL::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&SAGLinkLayerGSL::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&SAGLinkLayerGSL::m_bps),
                   MakeDataRateChecker ())
	.AddAttribute ("MaxFeederLinkNumber",
				   "Maximum number of feeder links",
				   UintegerValue (5),
				   MakeUintegerAccessor (&SAGLinkLayerGSL::m_maxFeederLinkNumber),
				   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&SAGLinkLayerGSL::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&SAGLinkLayerGSL::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&SAGLinkLayerGSL::m_queue),
                   MakePointerChecker<Queue<Packet> > ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
    //
    // Trace sources at the "bottom" of the net device, where packets transition
    // to/from the channel.
    //
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun "
                     "transmitting over the channel",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been "
                     "completely received by the device",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    // Note that there is really no difference between promiscuous and 
    // non-promiscuous traces in a point-to-point link.
    //
    .AddTraceSource ("Sniffer", 
                    "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&SAGLinkLayerGSL::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
	  //xhqin
	.AddAttribute ("TxPower",
					"Transmission output power in dBm.",
					DoubleValue (20),
					MakeDoubleAccessor (&SAGLinkLayerGSL::m_txPowerDb),
					MakeDoubleChecker<double> ())
	.AddAttribute ("TxAntennaGain",
					"Transmission antenna gain in dBi.",
					DoubleValue (20),
					MakeDoubleAccessor (&SAGLinkLayerGSL::m_txAntennaGain),
					MakeDoubleChecker<double> ())
	.AddAttribute ("Waveform",
					"Transmission Waveform in DVB-RCS2.",
					StringValue (""),
					MakeStringAccessor (&SAGLinkLayerGSL::m_waveformId),
					MakeStringChecker())
	.AddAttribute ("Protocol",
					"Transmission protocol.",
					StringValue (""),
					MakeStringAccessor (&SAGLinkLayerGSL::m_protocolString),
					MakeStringChecker ())
	.AddAttribute ("MCS",
					"Transmission modulation and coding system (MCS) type.",
					StringValue (""),
					MakeStringAccessor (&SAGLinkLayerGSL::m_modCodStringfwd),
					MakeStringChecker ())
	.AddAttribute ("BaseDir",
					"Directory",
					StringValue (""),
					MakeStringAccessor (&SAGLinkLayerGSL::m_baseDir),
					MakeStringChecker ())
	;
  return tid;
}


SAGLinkLayerGSL::SAGLinkLayerGSL () 
  :
    m_txMachineState (READY),
    m_channel (0),
    m_linkUp (false),
    m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
}

SAGLinkLayerGSL::~SAGLinkLayerGSL ()
{
  NS_LOG_FUNCTION (this);
}


//void
//SAGLinkLayerGSL::AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest)
//{
//  NS_LOG_FUNCTION (this << p << protocolNumber);
//  PppHeader ppp;
//  ppp.SetProtocol (EtherToPpp (protocolNumber));
//  p->AddHeader (ppp);
//}

//bool
//SAGLinkLayerGSL::ProcessHeader (Ptr<Packet> p, uint16_t& param)
//{
//  NS_LOG_FUNCTION (this << p << param);
//  PppHeader ppp;
//  p->RemoveHeader (ppp);
//  param = PppToEther (ppp.GetProtocol ());
//  return true;
//}

//Mengy's::1207替换
void
SAGLinkLayerGSL::AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  if(protocolNumber != 0x88cc)
  {
  PppHeader ppp;
  //Mengy's::modify
  ppp.SetProtocol (EtherToPpp (protocolNumber));
  p->AddHeader (ppp);
  }
}

//Mengy's::1207替换
bool
SAGLinkLayerGSL::ProcessHeader (Ptr<Packet> p, uint16_t& param)
{
  NS_LOG_FUNCTION (this << p << param);
  PppHeader ppp;

  p->PeekHeader(ppp);
  param = PppToEther (ppp.GetProtocol ());
  if(param != 0x86DD && param !=0x0800)
  {
	  param = 0x88cc;
  }
  else
  {
  p->RemoveHeader (ppp);
  //Mengy's::modify
  param = PppToEther (ppp.GetProtocol ());
  }
  return true;
}

void
SAGLinkLayerGSL::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  m_queue = 0;
  while (!m_queueDests.empty()) {
      m_queueDests.pop();
  }
  NetDevice::DoDispose ();
}

void SAGLinkLayerGSL::UpdateDataRate (){
	if(this->GetInstanceTypeId() == TypeId::LookupByName ("ns3::SAGLinkLayerGSL")){
		// just for fdma
		SetDataRate(m_bps.GetBitRate() / m_maxFeederLinkNumber);
//		if(m_channel->GetDevice(0)->GetNode()->GetId() != m_node->GetId()){
//			SetDataRate(m_bps.GetBitRate() / 5);
//		}
//		else{
//			SetDataRate(m_bps.GetBitRate());
//		}
	}
	else{
		SetDataRate(m_bps.GetBitRate());
	}
}

void
SAGLinkLayerGSL::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
SAGLinkLayerGSL::SetInterframeGap (Time t)  //frame interval ?
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

bool
SAGLinkLayerGSL::TransmitStart (Ptr<Packet> p, const Address dest)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  TrackUtilization(true);

//  Time txTime;
//  if(m_node->GetObject<SatellitePositionMobilityModel>() == nullptr){
//	  txTime = m_bps.CalculateBytesTxTime (p->GetSize ()) * (m_channel->GetNDevices()-1);
//  }
//  else{
//	  txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
//  }
  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;         //transmit delay

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  Simulator::Schedule (txCompleteTime, &SAGLinkLayerGSL::TransmitComplete, this, dest);

  bool result = m_channel->TransmitStart (p, this, dest, txTime);   //no frame interval ??
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }

  NS_LOG_FUNCTION (this << " done");
  return result;
}

void
SAGLinkLayerGSL::TransmitComplete (const Address dest)
{
  NS_LOG_FUNCTION (this);

  //
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "SAGLinkLayerGSL::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  TrackUtilization(false);
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

bool
SAGLinkLayerGSL::Attach (Ptr<SAGPhysicalLayerGSL> ch, bool isSat)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach (this, isSat);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  //NotifyStateToL3Stack();
  return true;
}

bool
SAGLinkLayerGSL::UnAttach (bool isSat)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (isSat != 1, "Cannot unattach satellite gsl device.");
  NS_ASSERT_MSG (m_channel != 0, "No Channel");

  m_channel->UnAttach (this, isSat);

  m_channel = 0;

  //m_linkUp = false;

  //NotifyStateToL3Stack();
  return true;
}

void
SAGLinkLayerGSL::NotifyStateToL3Stack ()
{
  //get l3 handle and notify disable
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  if(ipv4 != nullptr){
    int32_t intfid_v4 = ipv4->GetInterfaceForDevice(this);
    if(intfid_v4 != -1){ //unvalid interfaceid
      if (m_linkUp == false)
      {
        ipv4->SetDown(intfid_v4);
      }
      else if (m_linkUp == true)
      {
        ipv4->SetUp(intfid_v4);
      }
    }
  }

  Ptr<Ipv6> ipv6 = m_node->GetObject<Ipv6> ();
  if(ipv6 != nullptr){
    int32_t intfid_v6 = ipv6->GetInterfaceForDevice(this);
    if(intfid_v6 != -1){ //unvalid interfaceid
      if (m_linkUp == false)
      {
        ipv6->SetDown(intfid_v6);
      }
      else if (m_linkUp == true)
      {
        ipv6->SetUp(intfid_v6);
      }
    }
  }
}

void
SAGLinkLayerGSL::SetQueue (Ptr<Queue<Packet> > q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
SAGLinkLayerGSL::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
SAGLinkLayerGSL::Receive (Ptr<Packet> packet)
{
	NS_LOG_FUNCTION (this << packet << this->GetNode()->GetId());

	PerTag perInfo;
	double per = 0.0;
	if(packet->RemovePacketTag(perInfo)){
		per = perInfo.GetPer();
	}
	if(m_receiveErrorModel && m_receiveErrorModel->GetObject<RateErrorModel>()){
		m_receiveErrorModel->GetObject<RateErrorModel>()->SetRate(per);
	}

	uint16_t protocol = 0;

	if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
	{
		//
		// If we have an error model and it indicates that it is time to lose a
		// corrupted packet, don't forward this packet up, let it go.
		//
		m_phyRxDropTrace (packet);
	}
	else
	{
		//
		// Hit the trace hooks.  All of these hooks are in the same place in this
		// device because it is so simple, but this is not usually the case in
		// more complicated devices.
		//


		m_snifferTrace (packet);
		m_promiscSnifferTrace (packet);
		m_phyRxEndTrace (packet);

		//
		// Trace sinks will expect complete packets, not packets without some of the
		// headers.
		//
		Ptr<Packet> originalPacket = packet->Copy ();


		if(!ProcessHeader (packet, protocol)){
			// No need to upload
			return;
		}


//		if(this->GetNode()->GetId()==100 || this->GetNode()->GetId()==90){
//			std::cout<<Simulator::Now()<<"   "<<this->GetNode()->GetId()<<std::endl;
//		}


		if (!m_promiscCallback.IsNull ())
		{
		  m_macPromiscRxTrace (originalPacket);
		  //   the from address is incorrect (because it is unknown), but no effect is noticed on
		  // traffic reception
		  //   a solution would require changes to NS3 core to support send and receive that receive a
		  // 'from' argument for distributed simulator
		  m_promiscCallback (this, packet, protocol, GetAddress(), GetAddress (), NetDevice::PACKET_HOST);
		}

		m_macRxTrace (originalPacket);
		m_rxCallback (this, packet, protocol, GetAddress());
    }
}

Ptr<Queue<Packet> >
SAGLinkLayerGSL::GetQueue (void) const
{ 
  NS_LOG_FUNCTION (this);
  return m_queue;
}

double
SAGLinkLayerGSL::GetQueueOccupancyRate(){
	NS_LOG_FUNCTION (this);
	auto npkt = this->GetQueue()->GetNPackets();
	auto tkpt = this->GetQueue()->GetMaxSize().GetValue();
	return (double)npkt / tkpt * 100.0;
}

uint32_t
SAGLinkLayerGSL::GetMaxsize(){
	return this->GetQueue()->GetMaxSize().GetValue();
}

uint64_t
SAGLinkLayerGSL::GetDataRate (){
	return m_bps.GetBitRate();
}

void
SAGLinkLayerGSL::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
SAGLinkLayerGSL::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
SAGLinkLayerGSL::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
SAGLinkLayerGSL::GetChannel (void) const
{
  return m_channel;
}

void
SAGLinkLayerGSL::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
SAGLinkLayerGSL::GetAddress (void) const
{
  return m_address;
}

bool
SAGLinkLayerGSL::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
SAGLinkLayerGSL::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
SAGLinkLayerGSL::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true; // We return true such that the normal Internet stack can be installed, because ARP needs true here
}

Address
SAGLinkLayerGSL::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  //throw std::runtime_error("Broadcast not supported (only ARP would use broadcast, whose cache should have already been filled).");
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SAGLinkLayerGSL::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  // return false;
  return true; //support mulitcast
}

Address
SAGLinkLayerGSL::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  // throw std::runtime_error("Multicast not supported.");
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

Address
SAGLinkLayerGSL::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  // throw std::runtime_error("Multicast not supported.");
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SAGLinkLayerGSL::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
SAGLinkLayerGSL::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
SAGLinkLayerGSL::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());
  NS_LOG_LOGIC ("node is " << this->GetNode()->GetId());

  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  //
  // Stick a point to point protocol header on the packet in preparation for
  // shoving it out the door.
  //
  AddHeader (packet, protocolNumber, dest);

  //Broadcasts on GSL are currently truncated at the physical layer
  //std::cout << "SAG test:"+ std::to_string(this->GetNode()->GetId())+" Data Link Layer Send Pkt " << std::endl;

  m_macTxTrace (packet);

  //
  // We should enqueue and dequeue the packet to hit the tracing hooks.
  //
  if (m_queue->Enqueue (packet))
    {
        m_queueDests.push (dest);
      //
      // If the channel is ready for transition we send the packet right now
      // 
      if (m_txMachineState == READY)
        {
          packet = m_queue->Dequeue ();
          Address next_dest = m_queueDests.front ();
          m_queueDests.pop ();
          m_snifferTrace (packet);
          m_promiscSnifferTrace (packet);
          bool ret = TransmitStart (packet, next_dest);
          return ret;
        }
      return true;
    }

  // Enqueue may fail (overflow)

  m_macTxDropTrace (packet);
  return false;
}

bool
SAGLinkLayerGSL::SendFrom (Ptr<Packet> packet, 
                        const Address &source, 
                        const Address &dest, 
                        uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  return false;
}

Ptr<Node>
SAGLinkLayerGSL::GetNode (void) const
{
  return m_node;
}

void
SAGLinkLayerGSL::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
SAGLinkLayerGSL::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void
SAGLinkLayerGSL::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
SAGLinkLayerGSL::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
SAGLinkLayerGSL::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
SAGLinkLayerGSL::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

bool
SAGLinkLayerGSL::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
SAGLinkLayerGSL::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

uint16_t
SAGLinkLayerGSL::PppToEther (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
//    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    default:
     return 0;
    }
  return 0;
}

uint16_t
SAGLinkLayerGSL::EtherToPpp (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  switch(proto)
    {
    case 0x0800: return 0x0021;   //IPv4
    case 0x86DD: return 0x0057;   //IPv6
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}


void
SAGLinkLayerGSL::EnableUtilizationTracking(int64_t interval_ns) {
    m_utilization_tracking_enabled = true;
    m_interval_ns = interval_ns;
    m_prev_time_ns = 0;
    m_current_interval_start = 0;
    m_current_interval_end = m_interval_ns;
    m_idle_time_counter_ns = 0;
    m_busy_time_counter_ns = 0;
    m_current_state_is_on = false;
}

void
SAGLinkLayerGSL::TrackUtilization(bool next_state_is_on) {
    if (m_utilization_tracking_enabled) {

        // Current time in nanoseconds
        int64_t now_ns = Simulator::Now().GetNanoSeconds();
        while (now_ns >= m_current_interval_end) {

            // Add everything until the end of the interval
            if (next_state_is_on) {
                m_idle_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            } else {
                m_busy_time_counter_ns += m_current_interval_end - m_prev_time_ns;
            }

            // Save into the utilization array
            m_utilization.push_back(((double) m_busy_time_counter_ns) / ((double) m_interval_ns));

            // This must match up
            NS_ABORT_MSG_IF(m_idle_time_counter_ns + m_busy_time_counter_ns != m_interval_ns, "Not all time is accounted for");

            // Move to next interval
            m_idle_time_counter_ns = 0;
            m_busy_time_counter_ns = 0;
            m_prev_time_ns = m_current_interval_end;
            m_current_interval_start += m_interval_ns;
            m_current_interval_end += m_interval_ns;
        }

        // If not at the end of a new interval, just keep track of it all
        if (next_state_is_on) {
            m_idle_time_counter_ns += now_ns - m_prev_time_ns;
        } else {
            m_busy_time_counter_ns += now_ns - m_prev_time_ns;
        }

        // This has become the previous call
        m_current_state_is_on = next_state_is_on;
        m_prev_time_ns = now_ns;

    }
}



const std::vector<double>&
SAGLinkLayerGSL::FinalizeUtilization() {
    TrackUtilization(!m_current_state_is_on);
    return m_utilization;
}


//xhqin  attribute set and get
void
SAGLinkLayerGSL::SetTxPowerDb (double txpwr)
{
	m_txPowerDb = txpwr;
}
double
SAGLinkLayerGSL::GetTxPowerDb (void)
{
	return m_txPowerDb;
}
void
SAGLinkLayerGSL::SetTxAntennaGain (double txAntGain)
{
	m_txAntennaGain = txAntGain;
}
double
SAGLinkLayerGSL::GetTxAntennaGain (void)
{
	return m_txAntennaGain;
}
void
SAGLinkLayerGSL::SetWaveformId (double waveformId)
{
	m_waveformId = waveformId;
}
int
SAGLinkLayerGSL::GetWaveformId (void)
{
	return stoi(m_waveformId);
}
void
SAGLinkLayerGSL::SetTxProtocol (std::string protocolString)
{
	m_protocolString = protocolString;
}
std::string
SAGLinkLayerGSL::SetTxProtocol (void)
{
	return m_protocolString;
}
SatEnums::SatBbFrameType_t
SAGLinkLayerGSL::GetFrameType(void){
	if (m_protocol==DVB_S2X){
		std::string::size_type idx;
		idx=m_protocolString.find("NORMAL"); //在a中查找b.
		if (idx == std::string::npos ){
			return SatEnums::SHORT_FRAME;
		} //不存在。
		else{
			return SatEnums::NORMAL_FRAME;
		}
	}
	else{
		return SatEnums::UNDEFINED_FRAME;
	}
}
void
SAGLinkLayerGSL::SetTxMCS (std::string modCodStringfwd)
{
	m_modCodStringfwd = modCodStringfwd;
}
SatEnums::SatModcod_t
SAGLinkLayerGSL::GetTxMCS (void)
{
	//SatEnums::GetModcodFromName(m_modCodStringfwd)
	return SatEnums::GetModcodFromName(m_modCodStringfwd);
}
Ptr<CircularApertureAntennaModel>
SAGLinkLayerGSL::GetAntennaModel (){
	m_AntennaModel->SetMaxGain(m_txAntennaGain);
	//m_AntennaModel->SetOperatingFrequency(m_propagationFrequency);
	return m_AntennaModel;//CircularApertureAntennaModel
}
Ptr<SatLinkResultsRtn>
SAGLinkLayerGSL::GetLinkResultsRTN (void)
{
	m_linkresultrtn = CreateObject<SatLinkResultsDvbRcs2>();
	m_linkresultrtn->SetBaseDir(m_baseDir);
	m_linkresultrtn->Initialize();
	return m_linkresultrtn;
}
Ptr<SatLinkResultsFwd>
SAGLinkLayerGSL::GetLinkResultsFWD (void)
{
	SetProtocol(m_protocolString);
	m_linkresultfwd->Initialize();
	return m_linkresultfwd;
}
void
SAGLinkLayerGSL::SetProtocol (std::string str) //gs to satellite
{
  m_protocol = stringToEnum_pro(str);
  switch (m_protocol)
  {
    case DVB_S2:
      m_linkresultfwd= CreateObject<SatLinkResultsDvbS2>();
      m_linkresultfwd->SetBaseDir(m_baseDir);
      break;
    case DVB_S2X:
      m_linkresultfwd= CreateObject<SatLinkResultsDvbS2X>();
      m_linkresultfwd->SetBaseDir(m_baseDir);
      break;
    default:
      m_linkresultfwd= 0;
	  std::cout<<"The propagation protocol is not supported now:"<<str<<"!!!!!!"<<std::endl;
   }
}
//xhqin
TxProtocol
SAGLinkLayerGSL::GetProtocol(){
  return m_protocol;
}
//xhqin
TxProtocol
SAGLinkLayerGSL::stringToEnum_pro(std::string str){
	if (str == "DVB-S2") {
		return DVB_S2;
	} else if (str == "DVB-S2X") {
		return DVB_S2X;
	}
	else {
		return Unknown;
	}
}


} // namespace ns3

