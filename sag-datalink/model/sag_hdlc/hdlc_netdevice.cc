/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */


#include "hdlc_netdevice.h"

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
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-interface.h"
#include<ns3/hdlc_header.h>
#include "hdlc_channel.h"
#include "ns3/header.h"
#include "ns3/timer.h"
#define  wnd_ 10

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HdlcNetDevice");

NS_OBJECT_ENSURE_REGISTERED (HdlcNetDevice);

TypeId 
HdlcNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HdlcNetDevice")
    .SetParent<SAGLinkLayer> ()
    .SetGroupName ("Hdlc")
    .AddConstructor<HdlcNetDevice>()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&HdlcNetDevice::SetMtu,
                                         &HdlcNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&HdlcNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&HdlcNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&HdlcNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&HdlcNetDevice::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Transmit queueing discipline for the device which includes its own set
    // of trace hooks.
    //
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&HdlcNetDevice::m_queue),
                   MakePointerChecker<Queue<Packet> > ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
    //
    // Trace sources at the "bottom" of the net device, where packets transition
    // to/from the channel.
    //
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun "
                     "transmitting over the channel",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been "
                     "completely received by the device",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    // Note that there is really no difference between promiscuous and 
    // non-promiscuous traces in a point-to-point link.
    //
    .AddTraceSource ("Sniffer", 
                    "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer", 
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&HdlcNetDevice::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


HdlcNetDevice::HdlcNetDevice ()
  :
    m_txMachineState (READY),
    m_p2pLinkState (NORMAL),
    m_channel (0),
    m_linkUp (false),
    m_currentPkt (0)//	m_ARQstate()//	m_ARQstate(0,0,0,0,0)
{
  NS_LOG_FUNCTION (this);
  m_metaData[P2PInterruptionType::Unpredictable] = false;
  m_metaData[P2PInterruptionType::Predictable] = false;
  m_ARQstate = new ARQstate;  // 分配内存并初始化结构体实例
  m_ARQstate->recv_seqno_ = 0;
  m_ARQstate->SNRM_req_ = 0;
  m_ARQstate->receive_ack=0;
  m_ARQstate->m_packet=0;
  m_ARQstate->rece=0;
  m_ARQstate->m_Event=EventId();
}

HdlcNetDevice::~HdlcNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
HdlcNetDevice::SAGLinkDoSomethingWhenSend (Ptr<Packet> p)
{
  //for vartual
}

void
HdlcNetDevice::SAGLinkDoSomethingWhenReceive (Ptr<Packet> p)
{
  //for vartual
}

void
HdlcNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  m_queue = 0;
  NetDevice::DoDispose ();
}

void 
HdlcNetDevice::SetLinkState (P2PLinkState p2pLinkState)
{
  NS_LOG_FUNCTION (this);
  m_p2pLinkState = p2pLinkState;
  if (p2pLinkState == DISABLE)
  {
	  m_linkUp = false;
  }
  else if (p2pLinkState == NORMAL)
  {
	  m_linkUp = true;
  }
  NotifyStateToL3Stack(m_p2pLinkState);
}

void 
HdlcNetDevice::NotifyStateToL3Stack (P2PLinkState p2pLinkState)
{
  //get l3 handle and notify disable
  Ptr<Ipv4L3Protocol> ipv4L3 = m_node->GetObject<Ipv4L3Protocol> ();
  Ptr<Ipv6L3Protocol> ipv6L3 = m_node->GetObject<Ipv6L3Protocol> ();
  if (p2pLinkState == DISABLE)
  {
	ipv4L3->SetDown(ipv4L3->GetInterfaceForDevice(this));
    //ipv4L3->GetInterface(ipv4L3->GetInterfaceForDevice(this))->SetDown();
    //ipv6L3->GetInterface(ipv6L3->GetInterfaceForDevice(this))->SetDown(); // bug when run only with v6
  }
  else if (p2pLinkState == NORMAL)
  {
	ipv4L3->SetUp(ipv4L3->GetInterfaceForDevice(this));
    //ipv4L3->GetInterface(ipv4L3->GetInterfaceForDevice(this))->SetUp();
    //ipv6L3->GetInterface(ipv6L3->GetInterfaceForDevice(this))->SetUp();
  }
}

void
HdlcNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

uint64_t
HdlcNetDevice::GetDataRate (){
	return m_bps.GetBitRate();
}

void
HdlcNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

bool
HdlcNetDevice::TransmitStart (Ptr<Packet> p)
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
//  m_phyTxBeginTrace (m_currentPkt);
  TrackUtilization(true);

  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  Simulator::Schedule (txCompleteTime, &HdlcNetDevice::TransmitComplete, this);

  //std::cout<<m_destination_node->GetId()<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
  bool result = m_channel->GetObject<HdlcChannel>()->TransmitStart (p, this, m_destination_node, txTime);
  if (result == false)
    {
//      m_phyTxDropTrace (p);
    }
  return result;
}

void
HdlcNetDevice::TransmitComplete (void)
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

  NS_ASSERT_MSG (m_currentPkt != 0, "HdlcNetDevice::TransmitComplete(): m_currentPkt zero");

//  m_phyTxEndTrace (m_currentPkt);
  TrackUtilization(false);
  m_currentPkt = 0;

  //std::cout<<m_queue->GetNPackets()<<std::endl;
  Ptr<Packet> p = m_queue->Dequeue ();
  //std::cout<<m_queue->GetNPackets()<<std::endl;
  if (p == 0)
    {
      NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
      return;
    }

  //
  // Got another packet off of the queue, so start the transmit process again.
  //
//  m_snifferTrace (p);
//  m_promiscSnifferTrace (p);
  TransmitStart (p);
}

bool
HdlcNetDevice::Attach (Ptr<SAGPhysicalLayer> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch->GetObject<HdlcChannel>();

  m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

void
HdlcNetDevice::SetQueue (Ptr<Queue<Packet> > q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
HdlcNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
HdlcNetDevice::Receive (Ptr<Packet> packet)
{
//	std::cout<<"receive p"<<std::endl;
  NS_LOG_FUNCTION (this << packet);
  HdlcHeader hdlc;
  packet->RemoveHeader(hdlc);
  if(hdlc.GetHDLCFrameType()==HDLC_U_frame){
  if(hdlc.GetHDLCFrameType()==HDLC_U_frame && hdlc.GetHDLCFrameUType()==SNRM){
	//If no link is established, cache data and send SNRM frames to establish a link
	  SetLinkState (ESTABLISH);
	  Ptr<Packet> p = Create<Packet>();
	  HdlcHeader hdlcn;
	  hdlcn.SetHDLCFrameUType(UA);
	  hdlcn.SetHDLCFrameType(HDLC_U_frame);
	   p->AddHeader(hdlcn);
		if (m_txMachineState == READY && m_p2pLinkState == ESTABLISH)
		{
		  m_snifferTrace (p);
		  m_promiscSnifferTrace (p);
		  if(TransmitStart (p)){
		  SetLinkState (TRANSMIT);
		  std::cout<<"receive SNRM and send UA"<<std::endl;}
		}
		return;
  }
  if(hdlc.GetHDLCFrameType()==HDLC_U_frame && hdlc.GetHDLCFrameUType()==UA){
  	  SetLinkState (TRANSMIT);
  	  std::cout<<"receive  UA : transmit start"<<std::endl;
  	return;}

  if(hdlc.GetHDLCFrameType()==HDLC_U_frame && hdlc.GetHDLCFrameUType()==DISC){
	//If no link is established, cache data and send SNRM frames to establish a link
	  Ptr<Packet> p = Create<Packet>();
	  HdlcHeader hdlcn;
	  hdlcn.SetHDLCFrameUType(DM);
	  hdlcn.SetHDLCFrameType(HDLC_U_frame);
	   p->AddHeader(hdlcn);
	m_snifferTrace (p);
	m_promiscSnifferTrace (p);
	if(TransmitStart (p)){
	std::cout<<"receive DISC and send DM"<<std::endl;}
	 SetLinkState (DISABLE);
	 return;}

  if(hdlc.GetHDLCFrameType()==HDLC_U_frame && hdlc.GetHDLCFrameUType()==DM){
	  SetLinkState (DISABLE);
  	  std::cout<<"receive DM : transmit disable"<<std::endl;
  	return;}}

  else if(hdlc.GetHDLCFrameType()==HDLC_I_frame){
//	  std::cout<<"receive i packet"<<std::endl;
  if (m_p2pLinkState == DISABLE) return;

  uint16_t protocol = 0x0800;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      //
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
//      m_phyRxDropTrace (packet);
    }
  else
    {
      //
      // Hit the trace hooks.  All of these hooks are in the same place in this
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      //
//      m_snifferTrace (packet);
//      m_promiscSnifferTrace (packet);
//      m_phyRxEndTrace (packet);

      //
      // Trace sinks will expect complete packets, not packets without some of the
      // headers.
      //
      Ptr<Packet> originalPacket = packet->Copy ();

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
	Ptr<Packet> p = Create<Packet>();
	HdlcHeader hdlcn;
	hdlcn.SetHDLCFrameType(HDLC_S_frame);
	int seq;
	seq=m_ARQstate->recv_seqno_;
	seq=seq%8;
//	std::cout<<seq<<std::endl;
	hdlcn.SetHDLCReceseq(seq);
	hdlcn.SetHDLCFrameSType(RR);
	std::cout<<"receive packet "<<m_ARQstate-> recv_seqno_<<" send RR"<<std::endl;
	m_ARQstate->recv_seqno_++;
	p->AddHeader(hdlcn);
	m_snifferTrace (p);
	m_promiscSnifferTrace (p);
	if(TransmitStart (p)){}
      SAGLinkDoSomethingWhenReceive(packet);


//      }
      //std::cout<<m_node->GetId()<<"  "<<m_destination_node->GetId()<<std::endl;
      if (!m_promiscCallback.IsNull ())
        {
//          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (), NetDevice::PACKET_HOST);
        }

//      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, protocol, GetRemote ());
    }
  return;
  }
  else{
  if(hdlc.GetHDLCFrameType()==HDLC_S_frame && hdlc.GetHDLCFrameSType()==RR) {
	  std::cout<<"receive RR of packet"<<m_ARQstate->recv_seqno_<<std::endl;
	  m_ARQstate->recv_seqno_++;
	  }}
}
Ptr<Queue<Packet>>
HdlcNetDevice::GetQueue (void) const
{ 
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
HdlcNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
HdlcNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
HdlcNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
HdlcNetDevice::GetChannel (void) const
{
  return m_channel;
}

//
// This is a hdlc device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
HdlcNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
HdlcNetDevice::GetAddress (void) const
{
  return m_address;
}

void
HdlcNetDevice::SetDestinationNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_destination_node = node;
}

Ptr<Node>
HdlcNetDevice::GetDestinationNode (void) const
{
  return m_destination_node;
}

bool
HdlcNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
HdlcNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a hdlc device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
HdlcNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a 
// hdlc device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
HdlcNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
HdlcNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
HdlcNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
HdlcNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

double
HdlcNetDevice::GetQueueOccupancyRate(){
	NS_LOG_FUNCTION (this);
	auto npkt = this->GetQueue()->GetNPackets();
	auto tkpt = this->GetQueue()->GetMaxSize().GetValue();
//	if(npkt>0)
//	std::cout<<Simulator::Now().GetSeconds()<<"  "<<npkt<< "  "<< tkpt<<std::endl;
	return (double)npkt / tkpt * 100.0;
}

bool
HdlcNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
HdlcNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
HdlcNetDevice::SendDISC ()
{
		Ptr<Packet> p = Create<Packet>();
		HdlcHeader hdlcn;
		hdlcn.SetHDLCFrameUType(DISC);
		hdlcn.SetHDLCFrameType(HDLC_U_frame);
		p->AddHeader(hdlcn);
		  m_snifferTrace (p);
		  m_promiscSnifferTrace (p);
		  std::cout << "send DISC" << std::endl;
		  bool ret = TransmitStart (p);
		  if(ret){;}
		  SetLinkState(TERMINATE);
}

bool
HdlcNetDevice::Send (
  Ptr<Packet> packet, 
  const Address &dest, 
  uint16_t protocolNumber)
{

//	std::cout << "send p" << std::endl;
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());

  m_ARQstate->m_Event.Cancel();
  m_ARQstate->m_Event=Simulator::Schedule(Seconds(20),&HdlcNetDevice::SendDISC,this);

	if(m_ARQstate->SNRM_req_==0)
	{
		bool enq = m_queue->Enqueue (packet);
		if (enq){
		Ptr<Packet> p = Create<Packet>();
		HdlcHeader hdlcn;
		hdlcn.SetHDLCFrameUType(SNRM);
		hdlcn.SetHDLCFrameType(HDLC_U_frame);
		hdlcn.SetProtocol(protocolNumber);
		p->AddHeader(hdlcn);
		if (m_txMachineState == READY && m_p2pLinkState == NORMAL)
		{
		  m_snifferTrace (p);
		  m_promiscSnifferTrace (p);
		  m_ARQstate->SNRM_req_=1;
		  std::cout << "send SNRM!" << std::endl;
		  bool ret = TransmitStart (p);
		  SetLinkState(ESTABLISH);
		  return ret;}
		else{
			 return false;
		}
		}
		  else{
			  m_macTxDropTrace (packet);
			  return false;
		  }
	}

		if(m_p2pLinkState==TRANSMIT){
//		  m_macTxTrace (packet);;

		  SAGLinkDoSomethingWhenSend(packet);
		  //
		  // We should enqueue and dequeue the packet to hit the tracing hooks.
		  //

		  bool enq = m_queue->Enqueue (packet);
		  if (enq)
		    {
		    	  if(m_ARQstate->seqno_==m_ARQstate->recv_seqno_){
					packet = m_queue->Dequeue ();
					HdlcHeader hdlcn;
					hdlcn.SetHDLCFrameType(HDLC_I_frame);
					int seq;
					seq=m_ARQstate->seqno_;
					seq=seq%8;
					hdlcn.SetHDLCSendseq(seq);
					hdlcn.SetProtocol(protocolNumber);
					std::cout<<"send Iframe seqno:"<<m_ARQstate->seqno_<<std::endl;
					packet->AddHeader(hdlcn);
					m_snifferTrace (packet);
					m_promiscSnifferTrace (packet);
					bool ret = TransmitStart (packet);
					m_ARQstate->seqno_++;
					if(ret){
					m_ARQstate->m_packet=packet;
					return true;}
		  		}
//		    	  else{
//			    	  std::cout<<"no send m_ARQstate->seqno_ is"<<m_ARQstate->seqno_<<"m_ARQstate->recv_seqno_ is"<<m_ARQstate->recv_seqno_<<std::endl;}
//		    	  	  std::cout<<"resend"<<std::endl;
//						packet = m_ARQstate->m_packet;
//						HdlcHeader hdlcn;
//						hdlcn.SetHDLCFrameType(HDLC_I_frame);
//						hdlcn.SetHDLCSendseq(m_ARQstate->seqno_-1);
//						hdlcn.SetProtocol(protocolNumber);
//						std::cout<<"send Iframe seqno:"<<m_ARQstate->seqno_-1<<std::endl;
//						packet->AddHeader(hdlcn);
//						m_snifferTrace (packet);
//						m_promiscSnifferTrace (packet);
//						bool ret = TransmitStart (packet);
////						m_ARQstate->seqno_++;
//						if(ret){;}
////						m_ARQstate->m_packet=packet;
////						return true;}
//		        }
		      return true;
		  	std::cout << m_queue->GetNPackets() << std::endl;
		    }
		  else{
			  std::cout<<"m_p2pLinkState is not TRANSMIT"<<std::endl;
//			  m_macTxDropTrace (packet);
			  return false;
		  }
		}

////   Stick a point to point protocol header on the packet in preparation for
////   shoving it out the door.
		return false;
}

uint32_t
HdlcNetDevice::GetMaxsize(){
	return this->GetQueue()->GetMaxSize().GetValue();
}


bool
HdlcNetDevice::SendFrom (Ptr<Packet> packet,
                                 const Address &source, 
                                 const Address &dest, 
                                 uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  return false;
}

Ptr<Node>
HdlcNetDevice::GetNode (void) const
{
  return m_node;
}

void
HdlcNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
HdlcNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
HdlcNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
HdlcNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
HdlcNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
HdlcNetDevice::SetInterruptionInformation(P2PInterruptionType metaData, bool b){
	NS_LOG_FUNCTION (this);
	m_metaData[metaData] = b;
}

bool
HdlcNetDevice::GetInterruptionInformation(P2PInterruptionType metaData){
	NS_LOG_FUNCTION (this);
	return m_metaData[metaData];

}

void
HdlcNetDevice::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

Address 
HdlcNetDevice::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (std::size_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

bool
HdlcNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
HdlcNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

uint16_t
HdlcNetDevice::PppToEther (uint16_t proto)
{
  NS_LOG_FUNCTION_NOARGS();
  proto=0x0021;
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}

uint16_t
HdlcNetDevice::EtherToPpp (uint16_t proto)
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



} // namespace ns3
