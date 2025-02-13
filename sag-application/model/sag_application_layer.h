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

#ifndef SAG_APPLICATION_LAYER_H
#define SAG_APPLICATION_LAYER_H

#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/string.h"
#include "ns3/exp-util.h"
#include "ns3/sag_burst_info.h"
#include "ns3/syncodecs.h"
#include "ns3/sag_rtp_constants.h"
#include "ns3/basic-simulation.h"


namespace ns3 {

class Socket;
class Packet;

class SAGApplicationLayer : public Application
{
public:
    static TypeId GetTypeId (void);
    SAGApplicationLayer ();
    virtual ~SAGApplicationLayer ();

    virtual void SetBasicSimuAttr(Ptr<BasicSimulation> basicSimu);

    virtual void SetTransportFactory(std::string transport_factory_name);
    virtual void SetTransportSocket(std::string transport_socket_name);

    void SetDestinationNode(Ptr<Node> dst);
    Ptr<Node> GetDestinationNode();
    void SetSourceNode(Ptr<Node> src);
    Ptr<Node> GetSourceNode();
    Address GetDestinationAddress();
    Ipv4Address GetMyAddress();
    virtual void NotifyAddressChange();


    /**
       * \brief set the type of codec
       *
       * \param codecType
       */
    void SetCodecType (SyncodecType codecType);
    /**
       * \brief writes the header data to a buffer starting at the specified iterator position
       *
       * \param controller set the type of rtp controller
       */
    void SetTrace (TraceType traceType);

    std::shared_ptr<syncodecs::Codec> GetCodec(){
    	return m_codec;
    }

    void SetMaxPayLoadSizeByte(uint32_t max_payload_size_byte){
    	m_max_payload_size_byte = max_payload_size_byte;
    }

    void RecordDetailsLog(Ptr<Packet> pkt);
    void RecordRouteDetailsLog(std::vector<uint32_t> route);
    void RecordDetailsLogRouteOnly(Ptr<Packet> pkt);


    const std::vector<std::vector<uint32_t>>& GetRecordRouteDetailsLog(){
    	return m_routeDetailsLog;
    }

    const std::vector<int64_t>& GetRecordRouteDetailsTimeStampLogUs(){
    	return m_routeDetailsTimeStampLog_us;
    }

    const std::vector<int64_t>& GetRecordTimeStampLogUs(){
    	return m_recordTimeStampLog_us;
    }

    const std::vector<int64_t>& GetRecordProcessTimeStampLogUs(){
    	return m_recordProcessTimeStampLog_us;
    }

    const std::vector<double>& GetRecordDelaymsDetailsTimeStampLogUs(){
    	return m_delaymsDetailsTimeStampLog_us;
    }

    double GetMinDelayUs(){
    	return m_minDelay_us;
    }

    double GetMaxDelayUs(){
    	return m_maxDelay_us;
    }

    const std::vector<uint64_t>& GetRecordPktSizeBytes(){
    	return m_pktSizeBytes;
    }

    uint64_t GetTotalRxBytes(){
    	return m_totalRxBytes;
    }

    uint64_t GetTotalTxBytes(){
    	return m_totalTxBytes;
    }

    uint64_t GetTotalRxPacketNumber(){
    	return m_totalRxPacketNumber;
    }

    uint64_t GetTotalTxPacketNumber(){
    	return m_totalTxPacketNumber;
    }


protected:
    virtual void DoDispose (void);
    Ptr<Socket> m_socket; //!< Socket
    bool m_isIPv4Networking;		//<! ipv4
	bool m_isIPv6Networking;		//<! ipv6

    Ptr<Node> m_dstNode; //!< Record the destination node to find the updated destination address
    Ptr<Node> m_srcNode; //!< Record the srouce node to find the updated source address
    uint16_t m_dstPort; //!< Record the destination port

    std::string m_transport_factory_name; // let tcp enable use sag transport layer later!
    std::string m_transport_socket_name;  // change to socket factory instance later


    std::shared_ptr<syncodecs::Codec> m_codec;
    std::string m_baseLogsDir; //!< Where the burst logs will be written to:
                                //!<   logs_dir/burst_[id]_{incoming, outgoing}.csv
    std::string m_baseDir;
    std::string m_traceDir;
    std::string m_filePrefix; //!< The common prefix that all video trace files must have.
    float m_fps;  // frames-per-second
    uint32_t m_max_payload_size_byte;  //!< Maximum size of payload before getting fragmented

    // Logging
    std::vector<std::vector<uint32_t>> m_routeDetailsLog;
    std::vector<int64_t> m_routeDetailsTimeStampLog_us;  //<! us
    std::vector<int64_t> m_recordTimeStampLog_us;  //<! us
    std::vector<double> m_delaymsDetailsTimeStampLog_us;  //<! us
    double m_minDelay_us = std::numeric_limits<double>::max();	//<! us
	double m_maxDelay_us = 0;	//<! us
    std::vector<uint64_t> m_pktSizeBytes;
    std::vector<int64_t> m_recordProcessTimeStampLog_us;  //<! us

    uint64_t m_totalRxBytes = 0;     //!< Total bytes received
    uint64_t m_totalRxPacketNumber = 0;     //!< Total number of packets received
    uint64_t m_totalTxBytes = 0;     //!< Total bytes received
    uint64_t m_totalTxPacketNumber = 0;     //!< Total number of packets received

    bool m_enableDetailedLogging = false;

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);
    
    virtual void SetSocketType(TypeId tid, TypeId socketTid);
    virtual void HandleRead (Ptr<Socket> socket);
    double m_dynamicStateUpdateIntervalNs;

};


} // namespace ns3

#endif /* SAG_APPLICATION_H */
