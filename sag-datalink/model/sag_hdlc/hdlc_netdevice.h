/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author: Yuru Liu    November 2023
 *
 */

#ifndef POINT_TO_POINT_LASER_NET_DEVICE_H
#define POINT_TO_POINT_LASER_NET_DEVICE_H

#include <cstring>
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include <map>
#include "ns3/network-module.h" // 用于 Packet 类
#include<ns3/hdlc_header.h>
#include "ns3/header.h"
#include "ns3/sag_link_layer.h"
namespace ns3 {

template <typename Item> class Queue;

class HdlcChannel;
class ErrorModel;
class HdlcHeader;


struct ARQstate {
	int seqno_;
	int recv_seqno_;
	int receive_ack=0;
	Ptr<Packet> m_packet;// Cached packet
	int rece=0;
	int SNRM_req_;     // whether a SNRM request has been sent or not
	EventId m_Event;
};
/**
 * This HdlcNetDevice class specializes the NetDevice abstract
 * base class.  Together with a HdlcChannel (and a peer
 * HdlcNetDevice), the class models, with some level of
 * abstraction, a generic point-to-point-laser or serial link.
 * Key parameters or objects that can be specified for this device 
 * include a queue, data rate, and interframe transmission gap (the 
 * propagation delay is set in the HdlcChannel).
 */
class HdlcNetDevice : public SAGLinkLayer
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a HdlcNetDevice
   *
   * This is the constructor for the HdlcNetDevice
   */
  HdlcNetDevice ();

  /**
   * Destroy a HdlcNetDevice
   *
   * This is the destructor for the HdlcNetDevice.
   */
  virtual ~HdlcNetDevice ();

  /**
   * Set the Data Rate used for transmission of packets.  The data rate is
   * set in the Attach () method from the corresponding field in the channel
   * to which the device is attached.  It can be overridden using this method.
   *
   * \param bps the data rate at which this object operates
   */
  void SetDataRate (DataRate bps);

  uint64_t GetDataRate ();

  /**
   * Set the interframe gap used to separate packets.  The interframe gap
   * defines the minimum space required between packets sent by this device.
   *
   * \param t the interframe gap time
   */
  void SetInterframeGap (Time t);

  /**
   * Attach the device to a channel.
   *
   * \param ch Ptr to the channel to which this object is being attached.
   * \return true if the operation was successful (always true actually)
   */
  bool Attach (Ptr<SAGPhysicalLayer> ch);

  /**
   * Attach a queue to the HdlcNetDevice.
   *
   * The HdlcNetDevice "owns" a queue that implements a queueing
   * method such as DropTailQueue or RedQueue
   *
   * \param queue Ptr to the new queue.
   */
  void SetQueue (Ptr<Queue<Packet> > queue);

  /**
   * Get a copy of the attached Queue.
   *
   * \returns Ptr to the queue.
   */
  Ptr<Queue<Packet> > GetQueue (void) const;

  void SendDISC ();
  /**
   * Attach a receive ErrorModel to the HdlcNetDevice.
   *
   * \param em Ptr to the ErrorModel.
   */
  void SetReceiveErrorModel (Ptr<ErrorModel> em);
  /**
   * @brief Handles the reception of a packet by the HdlcNetDevice.
   *
   * This method is invoked when the HdlcNetDevice receives a packet from its connected channel.
   * It processes different HDLC frame types, such as SNRM, UA, DISC, and DM, to manage the link state.
   * For I-frames, it checks for sequential number.
   * The method also triggers trace hooks and forwards the packet up the protocol stack after processing.
   *
   * @param packet Ptr to the received packet.
   */

  void Receive (Ptr<Packet> p);

  // The remaining methods are documented in ns3::NetDevice*

  void SetLinkState (P2PLinkState p2pLinkState);

  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;

  virtual void SetDestinationNode (Ptr<Node> node);
  virtual Ptr<Node> GetDestinationNode (void) const;

  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;

  virtual bool IsLinkUp (void) const;

  virtual void AddLinkChangeCallback (Callback<void> callback);

  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;

  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;

  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  /**
   * Handles the transmission of a packet by the HdlcNetDevice.
   *
   * This method is responsible for sending packets based on the HDLC frame type and the current link state.
   * It supports different frame types such as SNRM, UA, DISC, DM, I-frame, and S-frame.
   * The method enqueues packets when the link is not established and sends the corresponding frame to establish the link.
   * For established links, it handles the transmission of I-frames and S-frames.
   * The method also triggers trace hooks and updates the link state accordingly.
   *
   * \param packet            Ptr to the packet to be transmitted.
   * \param dest              Destination address.
   * \param protocolNumber    Protocol number.
   * \return                 True if the packet is successfully transmitted, false otherwise.
   */

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

  virtual Address GetMulticast (Ipv6Address addr) const;
  double GetQueueOccupancyRate();
  uint32_t GetMaxsize();

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

  void SetInterruptionInformation(P2PInterruptionType metaData, bool b);
  bool GetInterruptionInformation(P2PInterruptionType metaData);

protected:
  /**
   * \brief Handler for MPI receive event
   *
   * \param p Packet received
   */
  void DoMpiReceive (Ptr<Packet> p);

private:

  /**
   * \brief Assign operator
   *
   * The method is private, so it is DISABLED.
   *
   * \param o Other NetDevice
   * \return New instance of the NetDevice
   */
  HdlcNetDevice& operator = (const HdlcNetDevice &o);

  /**
   * \brief Copy constructor
   *
   * The method is private, so it is DISABLED.

   * \param o Other NetDevice
   */
  HdlcNetDevice (const HdlcNetDevice &o);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:

  /**
   * \returns the address of the remote device connected to this device
   * through the point to point channel.
   */
  Address GetRemote (void) const;

  /**
   * For virtual do something when send
   * \param p packet
   */
  virtual void SAGLinkDoSomethingWhenSend (Ptr<Packet> p);

  /**
   * For virtual do something when receive
   * \param p packet
   */
  virtual void SAGLinkDoSomethingWhenReceive (Ptr<Packet> p);

  /**
   * Start Sending a Packet Down the Wire.
   *
   * The TransmitStart method is the method that is used internally in the
   * HdlcNetDevice to begin the process of sending a packet out on
   * the channel.  The corresponding method is called on the channel to let
   * it know that the physical device this class represents has virtually
   * started sending signals.  An event is scheduled for the time at which
   * the bits have been completely transmitted.
   *
   * \see HdlcChannel::TransmitStart ()
   * \see TransmitComplete()
   * \param p a reference to the packet to send
   * \returns true if success, false on failure
   */
  bool TransmitStart (Ptr<Packet> p);

  /**
   * Stop Sending a Packet Down the Wire and Begin the Interframe Gap.
   *
   * The TransmitComplete method is used internally to finish the process
   * of sending a packet out on the channel.
   */
  void TransmitComplete (void);

  /**
   * \brief Make the link up and running
   *
   * It calls also the linkChange callback.
   */
  void NotifyLinkUp (void);

  /**
   * Enumeration of the states of the transmit machine of the net device.
   */
  enum TxMachineState
  {
    READY,   /**< The transmitter is ready to begin transmission of a packet */
    BUSY,     /**< The transmitter is busy transmitting a packet */
  };
  /**
   * The state of the Net Device transmit state machine.
   */
  TxMachineState m_txMachineState;

  /**
   * \brief Notify p2p link state to l3 protocal
   *\param p2pLinkState p2p link state
   * It calls the L3 protocal.
   */
  void NotifyStateToL3Stack (P2PLinkState p2pLinkState);

  /**
   * The state of the Net Device link state .
   */
  P2PLinkState m_p2pLinkState;

  /**
   * The data rate that the Net Device uses to simulate packet transmission
   * timing.
   */
  DataRate       m_bps;

  /**
   * The interframe gap that the Net Device uses to throttle packet
   * transmission
   */
  Time           m_tInterframeGap;

  /**
   * The HdlcChannel to which this HdlcNetDevice
   * has been attached.
   */
  Ptr<SAGPhysicalLayer> m_channel;

  /**
   * The Queue which this HdlcNetDevice uses as a packet source.
   * Management of this Queue has been delegated to the HdlcNetDevice
   * and it has the responsibility for deletion.
   * \see class DropTailQueue
   */
  Ptr<Queue<Packet> > m_queue;

  /**
   * Error model for receive packet events
   */
  Ptr<ErrorModel> m_receiveErrorModel;

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * at the L3/L2 transition are dropped before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a promiscuous trace (which doesn't mean a lot here
   * in the point-to-point-laser device).
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a non-promiscuous trace (which doesn't mean a lot 
   * here in the point-to-point-laser device).
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * but are dropped before being forwarded up to higher layers (at the L2/L3 
   * transition).
   */
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;

  /**
   * The trace source fired when a packet begins the transmission process on
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet before it tries
   * to transmit it.
   */
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium -- when the simulated first bit(s) arrive.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   * This happens if the receiver is not enabled or the error model is active
   * and indicates that the packet is corrupt.
   */
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

  /**
   * A trace source that emulates a non-promiscuous protocol sniffer connected 
   * to the device.  Unlike your average everyday sniffer, this trace source 
   * will not fire on PACKET_OTHERHOST events.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device \c hard_start_xmit where 
   * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET 
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example, 
   * this would correspond to the point at which the packet is dispatched to 
   * packet sniffers in \c netif_receive_skb.
   */
  TracedCallback<Ptr<const Packet> > m_snifferTrace;

  /**
   * A trace source that emulates a promiscuous mode protocol sniffer connected
   * to the device.  This trace source fire on packets destined for any host
   * just like your average everyday packet sniffer.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device \c hard_start_xmit where 
   * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET 
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example, 
   * this would correspond to the point at which the packet is dispatched to 
   * packet sniffers in \c netif_receive_skb.
   */
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;

  Ptr<Node> m_node;              //!< Node owning this NetDevice
  Ptr<Node> m_destination_node;  //!< Node at the other end of the p2pLaserLink
  Mac48Address m_address;        //!< Mac48Address of this NetDevice
  NetDevice::ReceiveCallback m_rxCallback;   //!< Receive callback
  NetDevice::PromiscReceiveCallback m_promiscCallback;  //!< Receive callback
                                                        //   (promisc data)
  uint32_t m_ifIndex; //!< Index of the interface
  bool m_linkUp;      //!< Identify if the link is up or not
  TracedCallback<> m_linkChangeCallbacks;  //!< Callback for the link change event

  static const uint16_t DEFAULT_MTU = 1500; //!< Default MTU

  //!< First element: Regular interruption; Second element: Unpredictable interruption
  std::map<P2PInterruptionType, bool> m_metaData;

  /**
   * \brief The Maximum Transmission Unit
   *
   * This corresponds to the maximum 
   * number of bytes that can be transmitted as seen from higher layers.
   * This corresponds to the 1500 byte MTU size often seen on IP over 
   * Ethernet.
   */
  uint32_t m_mtu;

  Ptr<Packet> m_currentPkt; //!< Current packet processed

  //add
   ARQstate* m_ARQstate;


public:

  /**
   * \brief PPP to Ethernet protocol number mapping
   * \param protocol A PPP protocol number
   * \return The corresponding Ethernet protocol number
   */
  static uint16_t PppToEther (uint16_t protocol);

  /**
   * \brief Ethernet to PPP protocol number mapping
   * \param protocol An Ethernet protocol number
   * \return The corresponding PPP protocol number
   */
  static uint16_t EtherToPpp (uint16_t protocol);

//private:
//  bool m_utilization_tracking_enabled = false;
//  int64_t m_interval_ns;
//  int64_t m_prev_time_ns;
//  int64_t m_current_interval_start;
//  int64_t m_current_interval_end;
//  int64_t m_idle_time_counter_ns;
//  int64_t m_busy_time_counter_ns;
//  bool m_current_state_is_on;
//  std::vector<double> m_utilization;
//  void TrackUtilization(bool next_state_is_on);
//
//public:
//    void EnableUtilizationTracking(int64_t interval_ns);
//    const std::vector<double>& FinalizeUtilization();

};

} // namespace ns3

#endif /* POINT_TO_POINT_LASER_NET_DEVICE_H */
