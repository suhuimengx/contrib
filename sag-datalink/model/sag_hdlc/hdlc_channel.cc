/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 * 
 */


#include "hdlc_channel.h"
#include "ns3/core-module.h"
#include "ns3/hdlc_header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HdlcChannel");

NS_OBJECT_ENSURE_REGISTERED (HdlcChannel);

TypeId 
HdlcChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HdlcChannel")
    .SetParent<SAGPhysicalLayer> ()
    .SetGroupName ("Hdlc")
    .AddConstructor<HdlcChannel> ()
    .AddAttribute ("Delay", "Initial propagation delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&HdlcChannel::m_initial_delay),
                   MakeTimeChecker ())
    .AddAttribute ("PropagationSpeed", "Propagation speed through the channel",
                   DoubleValue (299792458.0),
                   MakeDoubleAccessor (&HdlcChannel::m_propagationSpeed),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet "
                     "from the HdlcChannel, used by the Animation "
                     "interface.",
                     MakeTraceSourceAccessor (&HdlcChannel::m_txrxPointToPoint),
                     "ns3::HdlcChannel::TxRxAnimationCallback")
  ;
  return tid;
}

HdlcChannel::HdlcChannel()
  :
	SAGPhysicalLayer (),
    m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
HdlcChannel::Attach (Ptr<SAGLinkLayer> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);

  m_link[m_nDevices++].m_src = device->GetObject<HdlcNetDevice>();
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
  if (m_nDevices == N_DEVICES)
    {
      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
    }
}

bool
HdlcChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<HdlcNetDevice> src,
  Ptr<Node> node_other_end,
  Time txTime)
{

  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);

  Ptr<Packet> p_copy = p->Copy();
  RouteHopCountTrace(src->GetNode()->GetId(), node_other_end->GetId(), p_copy);

  Ptr<MobilityModel> senderMobility = src->GetNode()->GetObject<MobilityModel>();
  Ptr<MobilityModel> receiverMobility = node_other_end->GetObject<MobilityModel>();
  Time delay = this->GetDelay(senderMobility, receiverMobility); 

  uint32_t wire = src == m_link[0].m_src ? 0 : 1;

  //std::cout<<m_link[wire].m_dst->GetNode()->GetId ()<<"  "<<txTime + delay<<std::endl;
  Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode()->GetId (),
                                  txTime + delay, &HdlcNetDevice::Receive,
                                  m_link[wire].m_dst, p_copy);

  // Call the tx anim callback on the net device
  m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + delay);
  return true;
}

void HdlcChannel::Establishconnect(Ptr<HdlcNetDevice> src, Ptr<Node> node_other_end, Time txTime)
{

}

void
HdlcChannel::SAGPhysicalDoSomeThing (Ptr<const Packet> p)
{
  //For vatural
}

std::size_t
HdlcChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<HdlcNetDevice>
HdlcChannel::GetHdlcDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  return m_link[i].m_src;
}

Ptr<NetDevice>
HdlcChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetHdlcDevice (i);
}

Time
HdlcChannel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  double seconds = distance / m_propagationSpeed;
  return Seconds (seconds);
}

Ptr<HdlcNetDevice>
HdlcChannel::GetSource (uint32_t i) const
{
  return m_link[i].m_src;
}

Ptr<HdlcNetDevice>
HdlcChannel::GetDestination (uint32_t i) const
{
  return m_link[i].m_dst;
}

bool
HdlcChannel::IsInitialized (void) const
{
  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}

} // namespace ns3
