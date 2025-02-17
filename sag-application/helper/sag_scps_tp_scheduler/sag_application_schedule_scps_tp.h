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
 */
#ifndef SAG_APPLICATION_SCHEDULER_SCPS_TP_H
#define SAG_APPLICATION_SCHEDULER_SCPS_TP_H

#include "ns3/sag_application_schedule.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/sag_application_layer_scps_tp_send.h"
#include "ns3/sag_application_layer_scps_tp_sink.h"


namespace ns3 {

class SAGApplicationSchedulerScpsTp : public SAGApplicationScheduler
{

public:
    SAGApplicationSchedulerScpsTp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    void WriteResults();

protected:
    std::vector<SAGBurstInfoScpsTp> m_schedule;

private:
    void StartNextFlow(int i);

    std::string m_flows_csv_filename;
    std::string m_flows_txt_filename;

    std::vector<std::pair<SAGBurstInfoScpsTp, Ptr<SAGApplicationLayerScpsTpSend>>> m_responsible_for_outgoing_bursts;
    std::vector<std::pair<SAGBurstInfoScpsTp, Ptr<SAGApplicationLayerScpsTpSink>>> m_responsible_for_incoming_bursts;
};

std::vector<SAGBurstInfoScpsTp> read_scps_tp_flow_schedule(
        const std::string& filename,
        Ptr<TopologySatelliteNetwork> topology,
        const int64_t simulation_end_time_ns
);

}

#endif /* SCPS_TP_FLOW_SCHEDULER_H */
