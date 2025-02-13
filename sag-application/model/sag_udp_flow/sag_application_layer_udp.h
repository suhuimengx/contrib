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

#ifndef SAG_APPLICATION_LAYER_UDP_H
#define SAG_APPLICATION_LAYER_UDP_H

#include "ns3/sag_application_layer.h"


namespace ns3 {

class Socket;
class Packet;

class SAGApplicationLayerUdp : public SAGApplicationLayer
{
public:
    static TypeId GetTypeId (void);
    SAGApplicationLayerUdp ();
    virtual ~SAGApplicationLayerUdp ();

    uint32_t GetMaxPayloadSizeByte();
    virtual void RegisterOutgoingBurst(SAGBurstInfoUdp burstInfo, Ptr<Node> targetNode, uint16_t my_port_num, uint16_t port, bool enable_precise_logging);
    virtual void RegisterIncomingBurst(SAGBurstInfoUdp burstInfo, uint16_t my_port_num, bool enable_precise_logging);

    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> GetOutgoingBurstsInformation();
    std::vector<std::tuple<SAGBurstInfoUdp, uint64_t>> GetIncomingBurstsInformation();
    uint64_t GetSentCounterOf(int64_t burst_id);
    uint64_t GetReceivedCounterOf(int64_t burst_id);
    uint64_t GetSentCounterSizeOf(int64_t burst_id);
    uint64_t GetReceivedCounterSizeOf(int64_t burst_id);


protected:
    virtual void DoDispose (void);
    //Ptr<Socket> m_socket; //!< IPv4 Socket

private:
    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void StartNextBurst();
    void BurstSendOut(size_t internal_burst_idx);
    void TransmitFullPacket(size_t internal_burst_idx, uint32_t payload_size_byte);

    void SetSocketType(TypeId tid, TypeId socketTid);
    void HandleRead (Ptr<Socket> socket);

    //virtual void DoSomeThingBeforeStartApplication(void);
    virtual void DoSomeThingWhenSendPkt(Ptr<Packet> packet);
    virtual void DoSomeThingWhenReceivePkt(Ptr<Packet> packet);

    //InetSocketAddress GetTargetValidInetSocketAddress(Ptr<Node> dstNode);

    uint16_t m_port;      //!< Port on which we listen for incoming packets.

    EventId m_startNextBurstEvent; //!< Event to start next burst

    // Outgoing bursts
    //std::vector<std::tuple<SAGBurstInfoUdp, InetSocketAddress>> m_outgoing_bursts; //!< Weakly ascending on start time list of bursts
    //!< We need to know the DST's current IP. We will improve this in the future. todo
    std::vector<std::tuple<SAGBurstInfoUdp, Ptr<Node>>> m_outgoing_bursts; //!< Weakly ascending on start time list of bursts
    std::vector<uint64_t> m_outgoing_bursts_packets_sent_counter; //!< Amount of packets sent out already for each burst
    std::vector<uint64_t> m_outgoing_bursts_packets_size_sent_counter; //!< Total packets size sent out already for each burst
    std::vector<EventId> m_outgoing_bursts_event_id; //!< Event ID of the outgoing burst send loop
    std::vector<bool> m_outgoing_bursts_enable_precise_logging; //!< True iff enable precise logging for each burst
    size_t m_next_internal_burst_idx; //!< Next burst index to send out


    // Incoming bursts
    std::vector<SAGBurstInfoUdp> m_incoming_bursts;
    std::map<int64_t, uint64_t> m_incoming_bursts_received_counter;       //!< Counter for how many packets received
    std::map<int64_t, uint64_t> m_incoming_bursts_size_received_counter;       //!< Counter for how many packets received
    std::map<int64_t, uint64_t> m_incoming_bursts_enable_precise_logging; //!< True iff enable precise logging for each burst

};


} // namespace ns3

#endif /* SAG_APPLICATION_H */
