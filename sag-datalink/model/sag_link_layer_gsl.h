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


#ifndef SAG_LINK_LAYER_GSL 
#define SAG_LINK_LAYER_GSL

#include <cstring>
#include <queue>
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
#include "ns3/node-container.h"
#include "ns3/satellite-position-mobility-model.h"

#include "ns3/propagation-loss-model.h"
#include "ns3/sag_link_results.h"
#include "ns3/sag_lookup_table.h"
#include "ns3/sag_waveform_conf.h"
#include "ns3/sag_enums.h"
#include "ns3/circular-aperture-antenna-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-v2v-propagation-loss-model.h"
#include "ns3/per_tag.h"

namespace ns3 {

template <typename Item> class Queue;
class SAGPhysicalLayerGSL;
class ErrorModel;
//xhqin
class SatLinkResultsDvbS2;
class SatLinkResultsDvbS2X;
class SatLinkResultsDvbRcs2;
class SatEnums;

//xhqin
enum TxProtocol
{
	DVB_S2,
	DVB_S2X,
	Unknown
};

/**
 * \class SAGLinkLayerGSL
 * \brief A Device for a GSL Network Link.
 *
 * This SAGLinkLayerGSL class specializes the NetDevice abstract
 * base class.  Together with a GSLChannel (and a peer 
 * GSLDevice), the class models, with some level of 
 * abstraction, a generic GSL.
 * Key parameters or objects that can be specified for this device 
 * include a queue, data rate, and interframe transmission gap (the 
 * propagation delay is set in the GSLChannel).
 */
class  SAGLinkLayerGSL: public NetDevice
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a SAGLinkLayerGSL
   *
   * This is the constructor for the SAGLinkLayerGSL
   */
  SAGLinkLayerGSL ();

  /**
   * Destroy a SAGLinkLayerGSL
   *
   * This is the destructor for the SAGLinkLayerGSL.
   */
  virtual ~SAGLinkLayerGSL ();

  /**
   * Set the Data Rate used for transmission of packets.  The data rate is
   * set in the Attach () method from the corresponding field in the channel
   * to which the device is attached.  It can be overridden using this method.
   *
   * \param bps the data rate at which this object operates
   */
  void UpdateDataRate ();
  void SetDataRate (DataRate bps);

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
  bool Attach (Ptr<SAGPhysicalLayerGSL> ch, bool isSat);

  /**
   * \brief Unattach the device with a channel.
   *
   * \param isSat Is it the satellite side
   *
   */
  bool UnAttach (bool isSat);

  /**
   * \brief Notify link state to L3 stack
   *
   */
  void NotifyStateToL3Stack ();

  /**
   * Attach a queue to the SAGLinkLayerGSL.
   *
   * The GSLtNetDevice "owns" a queue that implements a queueing 
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

  double GetQueueOccupancyRate();
  uint32_t GetMaxsize();

  uint64_t GetDataRate ();
  /**
   * Attach a receive ErrorModel to the SAGLinkLayerGSL.
   *
   * The SAGLinkLayerGSL may optionally include an ErrorModel in
   * the packet receive chain.
   *
   * \param em Ptr to the ErrorModel.
   */
  void SetReceiveErrorModel (Ptr<ErrorModel> em);

  /**
   * Receive a packet from a connected channel.
   *
   * The netdevice receives packets from its connected channel
   * and forwards them up the protocol stack.  This is the public method
   * used by the channel to indicate that the last bit of a packet has 
   * arrived at the device.
   *
   * \param p Ptr to the received packet.
   */
  virtual void Receive (Ptr<Packet> p);

  // The remaining methods are documented in ns3::NetDevice*

  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;

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

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;


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
  SAGLinkLayerGSL& operator = (const SAGLinkLayerGSL &o);

  /**
   * \brief Copy constructor
   *
   * The method is private, so it is DISABLED.

   * \param o Other NetDevice
   */
  SAGLinkLayerGSL (const SAGLinkLayerGSL &o);


protected:

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

  /**
   * Adds the necessary headers and trailers to a packet of data in order to
   * respect the protocol implemented by the agent.
   * \param p packet
   * \param protocolNumber protocol number
   */
  virtual void AddHeader (Ptr<Packet> p, uint16_t protocolNumber, Address dest);

  /**
   * Removes, from a packet of data, all headers and trailers that
   * relate to the protocol implemented by the agent
   * \param p Packet whose headers need to be processed
   * \param param An integer parameter that can be set by the function
   * \return Returns true if the packet should be forwarded up the
   * protocol stack.
   */
  virtual bool ProcessHeader (Ptr<Packet> p, uint16_t& param);

  /**
   * Start Sending a Packet Down the Wire.
   *
   * The TransmitStart method is the method that is used internally in the
   * SAGLinkLayerGSL to begin the process of sending a packet out on
   * the channel.  The corresponding method is called on the channel to let
   * it know that the physical device this class represents has virtually
   * started sending signals.  An event is scheduled for the time at which
   * the bits have been completely transmitted.
   *
   * \see GSLChannel::TransmitStart ()
   * \see TransmitComplete()
   * \param p a reference to the packet to send
   * \param address where the packet is to be sent
   * \returns true if success, false on failure
   */
  virtual bool TransmitStart (Ptr<Packet> p, const Address address);

  /**
   * Stop Sending a Packet Down the Wire and Begin the Interframe Gap.
   *
   * The TransmitComplete method is used internally to finish the process
   * of sending a packet out on the channel.
   */
  virtual void TransmitComplete (const Address destination);

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
    BUSY     /**< The transmitter is busy transmitting a packet */
  };

  /**
   * The state of the Net Device transmit state machine.
   */
  TxMachineState m_txMachineState;

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
   * The GSLChannel to which this SAGLinkLayerGSL has been
   * attached.
   */
  Ptr<SAGPhysicalLayerGSL> m_channel;

  /**
   * The Queue which this SAGLinkLayerGSL uses as a packet source.
   * Management of this Queue has been delegated to the SAGLinkLayerGSL
   * and it has the responsibility for deletion.
   * \see class DropTailQueue
   */
  Ptr<Queue<Packet> > m_queue;                            //one queue for now

    /**
     * The FIFO queue for the destination MAC addresses
     */
  std::queue<Address> m_queueDests;

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
   * in the GSL device).
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a non-promiscuous trace (which doesn't mean a lot 
   * here in the GSL device).
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

  Ptr<Node> m_node;                                     //!< Node owning this NetDevice
  Mac48Address m_address;                               //!< Mac48Address of this NetDevice
  NetDevice::ReceiveCallback m_rxCallback;              //!< Receive callback
  NetDevice::PromiscReceiveCallback m_promiscCallback;  //!< Receive callback
                                                        //   (promisc data)
  uint32_t m_ifIndex;                      //!< Index of the interface
  bool m_linkUp;                           //!< Identify if the link is up or not

  TracedCallback<> m_linkChangeCallbacks;  //!< Callback for the link change event

  static const uint16_t DEFAULT_MTU = 1500; //!< Default MTU
  uint16_t m_maxFeederLinkNumber;
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


protected:
  bool m_utilization_tracking_enabled = false;
  int64_t m_interval_ns;
  int64_t m_prev_time_ns;
  int64_t m_current_interval_start;
  int64_t m_current_interval_end;
  int64_t m_idle_time_counter_ns;
  int64_t m_busy_time_counter_ns;
  bool m_current_state_is_on;
  std::vector<double> m_utilization;
  void TrackUtilization(bool next_state_is_on);

public:
    void EnableUtilizationTracking(int64_t interval_ns);
    const std::vector<double>& FinalizeUtilization();

protected:

    //xhqin
    std::string m_waveformId;
    std::string m_protocolString;
    TxProtocol m_protocol;
    SatEnums::SatModcod_t m_modCodfwd; //transmit modcod   SatEnums::SAT_MODCOD_16APSK_3_TO_4
    std::string m_modCodStringfwd;
    //SatEnums::SatModcod_t m_modCodrtn=SatEnums::SatModcod_t::SAT_MODCOD_RCS2; //transmit modcod   SatEnums::SAT_MODCOD_16APSK_3_TO_4
    double m_txPowerDb; //transmit power
    double m_txAntennaGain;
    bool m_isSat;
    //double m_per=0;
    double flag=10;
    double flag1=1;
    Ptr<UniformRandomVariable> m_pg;

    Ptr<CircularApertureAntennaModel> m_AntennaModel = CreateObject<CircularApertureAntennaModel>();//CircularApertureAntennaModel
    Ptr<SatLinkResultsRtn> m_linkresultrtn;//from sat to gs
    Ptr<SatLinkResultsFwd> m_linkresultfwd; //from gs to satellite  dvb_s2/s2x

    std::string m_baseDir;

public:

    //xhqin
    /**
  	 * Set the transmit power.
  	 *
  	 * \param txpwr Final output transmission power, in dB.
  	 */
    void SetTxPowerDb (double txpwr);
    /**
     * Get the current transmit power, in dB.
     *
     * \return The transmit power.
     */
    double GetTxPowerDb (void);
    //attribute set and get
    void SetTxAntennaGain (double txAntGain);
    double GetTxAntennaGain (void);
    void SetWaveformId (double waveformId);
    int GetWaveformId (void);
    void SetTxProtocol (std::string protocolString);
    std::string SetTxProtocol (void);
    void SetTxMCS (std::string modCodStringfwd);
    SatEnums::SatModcod_t GetTxMCS (void);
    SatEnums::SatBbFrameType_t GetFrameType(void);
    Ptr<CircularApertureAntennaModel> GetAntennaModel (void);
    Ptr<SatLinkResultsRtn> GetLinkResultsRTN (void);
    Ptr<SatLinkResultsFwd> GetLinkResultsFWD (void);
    //xhqin
    TxProtocol stringToEnum_pro(std::string str);
    void SetProtocol (std::string str);
    TxProtocol GetProtocol();

     //virtual void ReceivewithPower (Ptr<Packet> p, double per);//, SatEnums::SatModcod_t modcod

};

} // namespace ns3

#endif /* GSL_NET_DEVICE_H */

