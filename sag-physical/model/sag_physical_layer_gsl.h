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


#ifndef SAG_PHYSICAL_LAYER_GSL 
#define SAG_PHYSICAL_LAYER_GSL

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/mobility-model.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/mac48-address.h"

#include "ns3/propagation-loss-model.h"
#include "ns3/sag_link_results.h"
#include "ns3/sag_lookup_table.h"
#include "ns3/sag_waveform_conf.h"
#include "ns3/sag_enums.h"
#include "ns3/circular-aperture-antenna-model.h"
#include "ns3/three-gpp-antenna-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "per_tag.h"

namespace ns3 {
//xhqin
enum PropScenario
{
  DENSE_URBAN,
  URBAN,
  SUBURBAN_AND_RURAL,
  UNKNOWN
};
//xhqin //Ground station ---> satellite
enum WireState
{
	IDLE,
	TRANSMITTING,
	PROPAGATING
};

class SAGLinkLayerGSL;
class Packet;
//xhqin
class SatLinkResultsDvbS2;
class SatLinkResultsDvbS2X;
class SatLinkResultsDvbRcs2;
class SatEnums;

class Mac48AddressHash : public std::unary_function<Mac48Address, size_t> {
    public:
        size_t operator() (Mac48Address const &x) const;
};

class SAGPhysicalLayerGSL : public Channel 
{
public:
  static TypeId GetTypeId (void);
  SAGPhysicalLayerGSL ();

  /**
   * \brief Maintain the broadcast channel model
   *
   * \param p		Packet
   * \param src		Source gsl Netdevice
   * \param dst_address		Destination mac address
   * \param txTime		Transmission time
   *
   */
  virtual bool TransmitStart (
          Ptr<const Packet> p,
          Ptr<SAGLinkLayerGSL> src,
		  Address dst_address,
          Time txTime
  );

  /**
   * \brief Schedule channel propagation events
   *
   * \param p		Packet
   * \param src		Source gsl Netdevice
   * \param dst_address		Destination mac address
   * \param txTime		Transmission time
   * \param isSameSystem		Whether in the same system
   *
   */
  virtual bool TransmitTo(
          Ptr<const Packet> p,
          Ptr<SAGLinkLayerGSL> srcNetDevice,
          Ptr<SAGLinkLayerGSL> dstNetDevice,
          Time txTime,
          bool isSameSystem
  );

  /**
   * \brief Bind channel instance for netdevice
   *
   * \param device		GSL netdevice
   * \param isSat		Is it the satellite side
   *
   */
  void Attach (Ptr<SAGLinkLayerGSL> device, bool isSat = 0);
  /**
   * \brief Unbind netdevice and channel instance when switchover occurs
   *
   * \param device		GSL netdevice
   * \param isSat		Is it the satellite side
   *
   */
  void UnAttach (Ptr<SAGLinkLayerGSL> device, bool isSat = 0);

  std::size_t GetNDevices (void) const;
  Ptr<NetDevice> GetDevice (std::size_t i) const;

  void SetChannelDelay(std::vector<Time> delays){
	  m_delays = delays;
  }

  WireState GetState ();

  void RouteHopCountTrace(uint32_t nodeid1, uint32_t nodeid2, Ptr<Packet> p);


  //xhqin
  void SetChannelSINR(std::vector<double> sinrs){
  	  m_SINRs = sinrs;
    }
  void SetChannelPERFWD(std::vector<double> PERFWDs){
	  m_PERFWD = PERFWDs;
    }
  void SetChannelPERRTN(std::vector<double> PERRTNs){
	  m_PERRTN = PERRTNs;
    }
  /**
      * Calculate the SINR of the received packet.
      *
      * \param p Ptr to the received packet.
      * \param rxPowerDb the received power.
      * \param ambNoiseDb the received noise.
      * \return The SINR in DB
      */
    double CalcSinrDb (double rxPowerDb, double ambNoiseDb) const;//SAGTxMode mode
    /**
      * Convert dB to kilopascals.
      *
      *   \f[{\rm{kPa}} = {10^{\frac{{{\rm{dB}}}}{{10}}}}\f]
      *
      * \param db Signal level in dB.
      * \return Sound pressure in kPa.
      */
    inline double DbToKp (double db) const
    {
  	  return std::pow (10, db / 10.0);
    }
    /**
     * Convert kilopascals to dB.
     *
     *   \f[{\rm{dB}} = 10{\log _{10}}{\rm{kPa}}\f]
     *
     * \param kp Sound pressure in kPa.
     * \return Signal level in dB.
     */
    inline double KpToDb (double kp) const
    {
        return 10 * std::log10 (kp);
    }
    /**
     * Calculate the packet error probability based on
     * SINR at the receiver and a tx mode.
     *
     * \param pkt Packet which is under consideration.
     * \param sinrDb SINR at receiver.
     * \param modcod TX mode used to transmit packet.
     * \return Probability of packet error.
     */
    //double CalcPer (double sinrDb, SatEnums::SatModcod_t modcod, std::string protocol);//, SAGTxMode mode


    void SetNoiseDbm (double NoiseDbm);
    double GetNoiseDbm (void);
    void SetBandwidth (double Bandwidth);
    double GetBandwidth (void);
    void SetPropFrequency (double propagationFrequency);
    double GetPropFrequency (void);
    bool GetEnableBER (void);


    //xhqin
    /**
     * Get the Propagation Scenario used for transmission of packets.
     */
    Ptr<ThreeGppPropagationLossModel> GetPropagationScenario ();
    PropScenario stringToEnum_scen(std::string str);

protected:
  Time GetDelay (Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;
  Time GetDelay (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const;

  //xhqin
  double GetSINR (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const;
  double GetPER (Ptr<SAGLinkLayerGSL> srcNetDevice, Ptr<SAGLinkLayerGSL> destNetDevice) const;

  Time   m_lowerBoundDelay;                   //!< Propagation delay which is
                                              //   used to give a minimum lookahead time to the
                                              //   distributed simulator (if it were enabled).
                                              //   See also: distributed-simulator-impl.cc (line 173 onwards)

  double m_propagationSpeedMetersPerSecond;   //!< Propagation speed on the channel (used to live calculate the delay
                                              //   for each packet which is sent over this channel.
protected:
  // Mac address to net device
  typedef sgi::hash_map<Mac48Address, Ptr<SAGLinkLayerGSL>, Mac48AddressHash> MacToNetDevice;
  typedef sgi::hash_map<Mac48Address, Ptr<SAGLinkLayerGSL>, Mac48AddressHash>::iterator MacToNetDeviceI;
  MacToNetDevice m_link;

  Ptr<SAGLinkLayerGSL> m_sat_net_device;
  std::vector<Ptr<SAGLinkLayerGSL>> m_ground_net_devices;
  std::vector<Time> m_delays;
  

  WireState m_state;

  //xhqin
  std::vector<double> m_SINRs;
  std::vector<double> m_PERFWD;
  std::vector<double> m_PERRTN;
  double m_propagationFrequency;              //!< Propagation Frequency on the channel (used to live calculate the delay
                                                  //   for each packet which is sent over this channel.
  double m_NoiseDbm; //noise power in db
  double m_bandwidth;//channel bandwidth in Hz
  std::string m_propScenString;
  PropScenario m_propScenario;
  bool m_enableBER;
  Ptr<ThreeGppNTNDenseUrbanPropagationLossModel> m_loss_DU=CreateObject<ThreeGppNTNDenseUrbanPropagationLossModel>();
  Ptr<ThreeGppNTNUrbanPropagationLossModel> m_loss_U=CreateObject<ThreeGppNTNUrbanPropagationLossModel>();
  Ptr<ThreeGppNTNSuburbanPropagationLossModel> m_loss_SU=CreateObject<ThreeGppNTNSuburbanPropagationLossModel>();
  double flag=1;


};

} // namespace ns3

#endif /* SAG_PHYSICAL_LAYER_GSL_H */
