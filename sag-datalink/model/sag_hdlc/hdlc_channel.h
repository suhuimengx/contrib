/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 * 
 */

#ifndef HDLC_CHANNEL_H
#define HDLC_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/hdlc_netdevice.h"
#include "ns3/sag_physical_layer.h"


namespace ns3 {

class HdlcNetDevice;
class Packet;
class SAGLinkLayer;

/**
 * \brief Hdlc Channel
 * Similar to point-to-point channel
 */
class HdlcChannel : public SAGPhysicalLayer
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Create a HdlcChannel
   * 
   */
  HdlcChannel ();

  /**
   * \brief Attach a given netdevice to this channel
   * 
   * \param device pointer to the netdevice to attach to the channel
   */
  void Attach (Ptr<SAGLinkLayer> device);

  /**
   * \brief Transmit a packet over this channel
   * 
   * \param p Packet to transmit
   * \param src source HdlcNetDevice
   * \param node_other_end node at the other end of the channel
   * \param txTime transmission time
   * \returns true if successful (always true)
   */
  bool TransmitStart (Ptr<const Packet> p, Ptr<HdlcNetDevice> src, Ptr<Node> node_other_end, Time txTime);

  /**
  connection establishment
  */
  void Establishconnect(Ptr<HdlcNetDevice> src, Ptr<Node> node_other_end, Time txTime);

  /**
   * \brief Write the traffic sent to each node (link utilization) to a stringstream 
   * 
   * \param str the string stream
   * \param node_id the source node of the traffic
   */
  void WriteTraffic(std::stringstream& str, uint32_t node_id);

  /**
   * \brief Get number of devices on this channel
   * 
   * \returns number of devices on this channel
   */
  virtual std::size_t GetNDevices (void) const;

  /**
   * \brief Get HdlcNetDevice corresponding to index i on this channel
   * 
   * \param i Index number of the device requested
   * 
   * \returns Ptr to HdlcNetDevice requested
   */
  Ptr<HdlcNetDevice> GetHdlcDevice (std::size_t i) const;

  /**
   * \brief Get NetDevice corresponding to index i on this channel
   * 
   * \param i Index number of the device requested
   * 
   * \returns Ptr to NetDevice requested
   */
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

protected:
  /**
   * \brief Get the delay between two nodes on this channel
   * 
   * \param senderMobility location of the sender
   * \param receiverMobility location of the receiver
   * 
   * \returns Time delay
   */
  virtual Time GetDelay (Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;

  /**
   * \brief For Subclass do something
   * 
   * \param p packet
   * 
   * \returns Time delay
   */
  virtual void SAGPhysicalDoSomeThing (Ptr<const Packet> p);

  /**
   * \brief Check to make sure the link is initialized
   * 
   * \returns true if initialized, asserts otherwise
   */
  bool IsInitialized (void) const;

  /**
   * \brief Get the source net-device 
   * 
   * \param i the link (direction) requested
   * 
   * \returns Ptr to source HdlcNetDevice for the
   *          specified link
   */
  Ptr<HdlcNetDevice> GetSource (uint32_t i) const;

  /**
   * \brief Get the destination net-device
   * 
   * \param i the link requested
   * \returns Ptr to destination HdlcNetDevice for
   *          the specified link
   */
  Ptr<HdlcNetDevice> GetDestination (uint32_t i) const;

  /**
   * \brief stores the number of bytes send between every pair of nodes 
   * 
   * \param wire the direction to where the bytes were sent
   * \param n_bytes the amount of bytes sent
   */
  void BookkeepBytes (uint32_t wire, uint32_t n_bytes);

  /**
   * TracedCallback signature for packet transmission animation events.
   *
   * \param [in] packet The packet being transmitted.
   * \param [in] txDevice the TransmitTing NetDevice.
   * \param [in] rxDevice the Receiving NetDevice.
   * \param [in] duration The amount of time to transmit the packet.
   * \param [in] lastBitTime Last bit receive time (relative to now)
   * \deprecated The non-const \c Ptr<NetDevice> argument is deprecated
   * and will be changed to \c Ptr<const NetDevice> in a future release.
   */
  typedef void (* TxRxAnimationCallback)
    (Ptr<const Packet> packet,
     Ptr<NetDevice> txDevice, Ptr<NetDevice> rxDevice,
     Time duration, Time lastBitTime);
                    
private:
  /** Each point to point link has exactly two net devices. */
  static const std::size_t N_DEVICES = 2;

  Time               m_initial_delay;     //!< Propagation delay at the initial distance
                                          //   used to give a delay estimate to the
                                          //   distributed simulator
  double             m_propagationSpeed;  //!< propagation speed on the channel
  std::size_t        m_nDevices;          //!< Devices of this channel

  /**
   * The trace source for the packet transmission animation events that the 
   * device can fire.
   * Arguments to the callback are the packet, transmitting
   * net device, receiving net device, transmission time and 
   * packet receipt time.
   *
   * \see class CallBackTraceSource
   * \deprecated The non-const \c Ptr<NetDevice> argument is deprecated
   * and will be changed to \c Ptr<const NetDevice> in a future release.
   */
  TracedCallback<Ptr<const Packet>,     // Packet being transmitted
                 Ptr<NetDevice>,  // Transmitting NetDevice
                 Ptr<NetDevice>,  // Receiving NetDevice
                 Time,                  // Amount of time to transmit the pkt
                 Time                   // Last bit receive time (relative to now)
                 > m_txrxPointToPoint;

  /** \brief Wire states
   *
   */
  enum WireState
  {
    /** Initializing state */
    INITIALIZING,
    /** Idle state (no transmission from NetDevice) */
    IDLE,
    /** Transmitting state (data being transmitted from NetDevice. */
    TRANSMITTING,
  };

  /**
   * \brief Wire model for the HdlcChannel
   */
  class Link
  {
public:
    /** \brief Create the link, it will be in INITIALIZING state
     *
     */
    Link() : m_state (INITIALIZING), m_src (0), m_dst (0) {}

    WireState                       m_state; //!< State of the link
    Ptr<HdlcNetDevice> m_src;   //!< First NetDevice
    Ptr<HdlcNetDevice> m_dst;   //!< Second NetDevice
  };

	Link m_link[N_DEVICES]; //!< Link model
};

} // namespace ns3

#endif /* POINT_TO_POINT_LASER_CHANNEL_H */
