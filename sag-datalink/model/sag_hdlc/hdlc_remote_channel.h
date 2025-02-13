/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */


#ifndef HDLC_REMOTE_CHANNEL_H
#define HDLC_REMOTE_CHANNEL_H

#include "hdlc_channel.h"

namespace ns3 {

/**
 * \ingroup point-to-point
 *
 * \brief A Remote Point-To-Laser-Point Channel
 * 
 * This object connects two point-to-point-laser net devices where at least
 * one is not local to this simulator object. It simply override the transmit
 * method and uses an MPI Send operation instead.
 */
class HdlcRemoteChannel : public HdlcChannel
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /** 
   * \brief Constructor
   */
  HdlcRemoteChannel ();

  /** 
   * \brief Destructor
   */
  ~HdlcRemoteChannel ();

  /**
   * \brief Transmit the packet
   *
   * \param p Packet to transmit
   * \param src Source HdlcNetDevice
   * \param txTime Transmit time to apply
   * 
   * \returns true if successful (always true)
   */
  virtual bool TransmitStart (Ptr<const Packet> p, Ptr<HdlcNetDevice> src,
                              Ptr<Node> node_other_end, Time txTime);
};

} // namespace ns3

#endif
