/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */

#include <iostream>

#include "hdlc_netdevice.h"
#include "hdlc_remote_channel.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/mpi-interface.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HdlcRemoteChannel");

NS_OBJECT_ENSURE_REGISTERED (HdlcRemoteChannel);

TypeId
HdlcRemoteChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HdlcRemoteChannel")
    .SetParent<HdlcChannel> ()
    .SetGroupName ("Hdlc")
    .AddConstructor<HdlcRemoteChannel> ()
  ;
  return tid;
}

HdlcRemoteChannel::HdlcRemoteChannel ()
  : HdlcChannel ()
{
}

HdlcRemoteChannel::~HdlcRemoteChannel ()
{
}

bool
HdlcRemoteChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<HdlcNetDevice> src,
  Ptr<Node> node_other_end,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  IsInitialized ();

  Ptr<Packet> p_copy = p->Copy();
  RouteHopCountTrace(src->GetNode()->GetId(), node_other_end->GetId(), p_copy);

  Ptr<MobilityModel> senderMobility = src->GetNode()->GetObject<MobilityModel>();
  Ptr<MobilityModel> receiverMobility = node_other_end->GetObject<MobilityModel>();
  Time delay = this->GetDelay(senderMobility, receiverMobility);

  uint32_t wire = src == GetSource (0) ? 0 : 1;
  Ptr<HdlcNetDevice> dst = GetDestination (wire);

#ifdef NS3_MPI
  // Calculate the rxTime (absolute)
  Time rxTime = Simulator::Now () + txTime + delay;
  MpiInterface::SendPacket (p_copy, rxTime, dst->GetNode()->GetId (), dst->GetIfIndex());
#else
  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
  return true;
}

} // namespace ns3
