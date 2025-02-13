/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 ETH Zurich
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
 */
#ifndef SAG_APPLICATION_SCHEDULER_THREEGPPHTTP_H
#define SAG_APPLICATION_SCHEDULER_THREEGPPHTTP_H


#include "ns3/sag_application_schedule.h"
#include "ns3/sag_application_layer_udp.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"


namespace ns3 {

class SAGApplicationSchedulerThreeGppHttp : public SAGApplicationScheduler
{
public:
	SAGApplicationSchedulerThreeGppHttp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    void WriteResults();


protected:
    std::vector<SAGBurstInfo3GppHttp> m_schedule;

private:

    std::string m_flows_csv_filename;
    std::string m_flows_txt_filename;

    std::vector<std::pair<SAGBurstInfo3GppHttp, Ptr<ThreeGppHttpClient>>> m_responsible_for_outgoing_bursts;
    std::vector<std::pair<SAGBurstInfo3GppHttp, Ptr<ThreeGppHttpServer>>> m_responsible_for_incoming_bursts;


//    void ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>);
//
//    void MainObjectGenerated (uint32_t size);
//
//    void EmbeddedObjectGenerated (uint32_t size);
//
//    void ServerTx (Ptr<const Packet> packet);
//
//    void ClientRx (Ptr<const Packet> packet, const Address &address);
//
//    void ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet);
//
//    void ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet);
//
//    void CompleteObjectRxRTT(const Time & t, const Address &address);
//
//    void CompleteObjectRxDelay(const Time & t, const Address &address);

};

std::vector<SAGBurstInfo3GppHttp> read_3GppHttp_flow_schedule(
        const std::string& filename,
        Ptr<TopologySatelliteNetwork> topology,
        const int64_t simulation_end_time_ns
);





}


#endif /* SAG_APPLICATION_SCHEDULER_THREEGPPHTTP_H */
