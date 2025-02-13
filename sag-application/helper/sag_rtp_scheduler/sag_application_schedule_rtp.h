#ifndef SAG_APPLICATION_SCHEDULER_RTP_H
#define SAG_APPLICATION_SCHEDULER_RTP_H

#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstring>
#include <fstream>
#include <cinttypes>
#include <algorithm>
#include <regex>
#include "ns3/exp-util.h"
#include "ns3/topology.h"

#include "ns3/sag_application_schedule.h"
#include "ns3/sag_burst_info.h"
#include "ns3/sag_application_layer_rtp_sender.h"
#include "ns3/sag_application_layer_rtp_receiver.h"
#include "ns3/topology-satellite-network.h"

//change all SAGApplicationLayerUdp into SAGApplicationLayerRTP

//class SAGBurstInfoRtp;

//class SAGApplicationLayerRTPSender;

namespace ns3 {

class SAGApplicationSchedulerRtp : public SAGApplicationScheduler
{

public:
    SAGApplicationSchedulerRtp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology);
    /**
       * \brief write the results of the SAG application to the base type
       */
    void WriteResults();
    /**
       * \brief start the next flow
       *
       * \param i the ID of the flow to be sent
       */
    void StartNextFlow(int i);

protected:
    std::vector<SAGBurstInfoRtp> m_schedule;

private:
//    std::string m_sag_bursts_outgoing_csv_filename_rtp;
    std::string m_sag_bursts_outgoing_txt_filename_rtp;
//    std::string m_sag_bursts_incoming_csv_filename_rtp;
    std::string m_sag_bursts_incoming_txt_filename_rtp;

//    std::string m_flows_csv_filename;
//    std::string m_flows_txt_filename;



    std::vector<std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>>> m_responsible_for_outgoing_bursts_rtp;
    std::vector<std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>>> m_responsible_for_incoming_bursts_rtp;

};

std::vector<SAGBurstInfoRtp> read_sag_applicaton_schedule_rtp(
        const std::string& filename,
        Ptr<TopologySatelliteNetwork> topology,
        const int64_t simulation_end_time_ns);

}


#endif /* SAG_APPLICATION_SCHEDULER_H */


