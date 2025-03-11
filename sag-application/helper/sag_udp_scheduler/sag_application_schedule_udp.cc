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

#include "sag_application_schedule_udp.h"

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
#include "ns3/statistic.h"
#include "ns3/cppjson2structure.hh"

namespace ns3 {

SAGApplicationSchedulerUdp::SAGApplicationSchedulerUdp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
:SAGApplicationScheduler(basicSimulation, topology)
{
    printf("APPLICATION SCHEDULER UDP\n");

    // Check if it is enabled explicitly
    //m_enabled = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sag_application_scheduler_udp", "false"));
    m_enabled = ReadApplicationEnable("udp_flow");
    if (!m_enabled) {
        std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

    } else {
        std::cout << "  > SAG applicaiton scheduler is enabled" << std::endl;

        // Read logging
        ReadLoggingForApplicationIds("udp");

        // Read schedule
        m_schedule = read_sag_applicaton_schedule(
                m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_udp.json",
                m_topology,
                m_simulation_end_time_ns
        );

        // Check that the SAG applicaiton IDs exist in the logging
        if(!m_enable_logging_for_sag_application_ids.empty()){
        	for (int64_t sag_application_id : m_enable_logging_for_sag_application_ids) {
				if ((size_t) sag_application_id >= m_schedule.size()) {
					throw std::invalid_argument("Invalid SAG burst ID in sag_applicaiton_enable_logging_for_sag_application_udp_ids: " + std::to_string(sag_application_id));
				}
				if(m_system_id == 0){
		    		mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(sag_application_id));
				}
			}
        }

        // Schedule read
        printf("  > Read schedule (total SAG applications: %lu)\n", m_schedule.size());
        m_basicSimulation->RegisterTimestamp("Read SAG application schedule");

        // Determine filenames
        m_sag_bursts_outgoing_csv_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_bursts_outgoing_info.csv";
        m_sag_bursts_outgoing_txt_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_bursts_outgoing_info.txt";
        m_sag_bursts_incoming_csv_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_bursts_incoming_info.csv";
        m_sag_bursts_incoming_txt_filename = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_bursts_incoming_info.txt";

        // Remove files if they are there
        remove_file_if_exists(m_sag_bursts_outgoing_csv_filename);
        remove_file_if_exists(m_sag_bursts_outgoing_txt_filename);
        remove_file_if_exists(m_sag_bursts_incoming_csv_filename);
        remove_file_if_exists(m_sag_bursts_incoming_txt_filename);
        printf("  > Removed previous SAG application log files if present\n");
        m_basicSimulation->RegisterTimestamp("Remove previous SAG application log files");

        // Install sink on endpoint node
        uint16_t my_port_num = 1026;
        uint16_t dst_port_num = 1026;

        std::cout << "  > Setting up applications on endpoint nodes" << std::endl;
        for (SAGBurstInfoUdp entry : m_schedule) {
        	int64_t src = entry.GetFromNodeId();
        	int64_t dst = entry.GetToNodeId();
        	std::string traceType = entry.GetMetadata();
        	std::string codecType = entry.GetAdditionalParameters();

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[src] == m_system_id){
        		SAGApplicationHelperUdp sagApplicationHelperUdp(m_basicSimulation, codecType, traceType);
                // Setup the application
				ApplicationContainer app = sagApplicationHelperUdp.Install(m_nodes.Get(src));
				app.Start(Seconds(0.0));
				m_apps.push_back(app);

				// Register all bursts being sent from there and being received
				Ptr<SAGApplicationLayerUdp> sagApplicationLayerUdp = app.Get(0)->GetObject<SAGApplicationLayerUdp>();
                sagApplicationLayerUdp->RegisterOutgoingBurst(
                        entry,
                        m_nodes.Get(entry.GetToNodeId()),
						my_port_num,
						dst_port_num,
                        m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
                );
                m_responsible_for_outgoing_bursts.push_back(std::make_pair(entry, sagApplicationLayerUdp));

				// must be both set in sinks and senders
                sagApplicationLayerUdp->SetBasicSimuAttr(m_basicSimulation);
                sagApplicationLayerUdp->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
                sagApplicationLayerUdp->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));
        	}

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[dst] == m_system_id){
        		SAGApplicationHelperUdp sagApplicationHelperUdp(m_basicSimulation);
				// Setup the application
				ApplicationContainer app = sagApplicationHelperUdp.Install(m_nodes.Get(dst));
				app.Start(Seconds(0.0));
				m_apps.push_back(app);

				// Register all bursts being sent from there and being received
				Ptr<SAGApplicationLayerUdp> sagApplicationLayerUdp = app.Get(0)->GetObject<SAGApplicationLayerUdp>();
				sagApplicationLayerUdp->RegisterIncomingBurst(
					   entry,
					   my_port_num,
					   m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
				);
				m_responsible_for_incoming_bursts.push_back(std::make_pair(entry, sagApplicationLayerUdp));

				// must be both set in sinks and senders
                sagApplicationLayerUdp->SetBasicSimuAttr(m_basicSimulation);
                sagApplicationLayerUdp->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
                sagApplicationLayerUdp->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));
        	}
        	else{
				// Setup the virtual application, just for minimum hop routing
        		SAGApplicationHelper sagApplicationHelper(m_basicSimulation);
				ApplicationContainer app = sagApplicationHelper.Install(m_nodes.Get(dst));
				Ptr<SAGApplicationLayer> sagApplicationLayer = app.Get(0)->GetObject<SAGApplicationLayer>();

				// must be both set in sinks and senders
				sagApplicationLayer->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
				sagApplicationLayer->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));

        	}

        	my_port_num++;
        	dst_port_num++;


        }

        m_basicSimulation->RegisterTimestamp("Setup applications on endpoint nodes");

    }

    std::cout << std::endl;
}

void SAGApplicationSchedulerUdp::WriteResults() 
{
    std::cout << "STORE SAG APPLICATION RESULTS" << std::endl;

    // Check if it is enabled explicitly
    if (!m_enabled) {
        std::cout << "  > Not enabled, so no SAG application results are written" << std::endl;

    } else {

        // Open files
        std::cout << "  > Opening SAG application log files:" << std::endl;
        FILE* file_outgoing_csv = fopen(m_sag_bursts_outgoing_csv_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_sag_bursts_outgoing_csv_filename << std::endl;
        FILE* file_outgoing_txt = fopen(m_sag_bursts_outgoing_txt_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_sag_bursts_outgoing_txt_filename << std::endl;
        FILE* file_incoming_csv = fopen(m_sag_bursts_incoming_csv_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_sag_bursts_incoming_csv_filename << std::endl;
        FILE* file_incoming_txt = fopen(m_sag_bursts_incoming_txt_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_sag_bursts_incoming_txt_filename << std::endl;

        // Header
        std::cout << "  > Writing sag_application_{incoming, outgoing}.txt headers" << std::endl;
        fprintf(
                file_outgoing_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-16s%-28s%-28s%s\n",
                "Applicartion ID", "From", "To", "Target rate", "Start time", "Duration",
                "Outgoing rate (w/ headers)", "Outgoing rate (payload)", "Packets sent",
                "Data sent (w/headers)", "Data sent (payload)", "Metadata"
        );
        fprintf(
                file_incoming_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-19s%-28s%-28s%s\n",
                "Applicartion ID", "From", "To", "Target rate", "Start time", "Duration",
                "Incoming rate (w/ headers)", "Incoming rate (payload)", "Packets received",
                "Data received (w/headers)", "Data received (payload)", "Metadata"
        );

        // Sort ascending to preserve SAG applicartion schedule order
        struct ascending_paired_sag_application_id_key
        {
            inline bool operator() (const std::pair<SAGBurstInfoUdp, Ptr<SAGApplicationLayerUdp>>& a, const std::pair<SAGBurstInfoUdp, Ptr<SAGApplicationLayerUdp>>& b)
            {
                return (a.first.GetBurstId() < b.first.GetBurstId());
            }
        };
        std::sort(m_responsible_for_outgoing_bursts.begin(), m_responsible_for_outgoing_bursts.end(), ascending_paired_sag_application_id_key());
        std::sort(m_responsible_for_incoming_bursts.begin(), m_responsible_for_incoming_bursts.end(), ascending_paired_sag_application_id_key());


        JulianDate start_time_jd = m_nodes.Get(0)->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
        for (std::pair<SAGBurstInfoUdp, Ptr<SAGApplicationLayerUdp>> p : m_responsible_for_incoming_bursts) {

            SAGBurstInfoUdp info = p.first;
            Ptr<SAGApplicationLayerUdp> sagApplicationUdpIncoming = p.second;
            if(m_enable_logging_for_sag_application_ids.find(info.GetBurstId()) == m_enable_logging_for_sag_application_ids.end()){
            	continue;
            }
    		//mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(info.GetBurstId()));

            // Flow Details
            const std::vector<std::vector<uint32_t>> routes = sagApplicationUdpIncoming->GetRecordRouteDetailsLog();
            if(routes.size() == 0){
            	continue;
            }
            const std::vector<int64_t> route_timestamp =  sagApplicationUdpIncoming->GetRecordRouteDetailsTimeStampLogUs();
            uint32_t path_change_number = routes.size() - 1;
            std::vector<uint32_t> path_hop_count;
            for(auto path: routes){
            	path_hop_count.push_back(path.size());
            }
            auto max_it = std::max_element(path_hop_count.begin(), path_hop_count.end());
            auto min_it = std::min_element(path_hop_count.begin(), path_hop_count.end());
            uint32_t path_hop_count_maxdif = *max_it - *min_it;

            const std::vector<int64_t> record_timestamp =  sagApplicationUdpIncoming->GetRecordTimeStampLogUs();
            const std::vector<double> pkt_delay =  sagApplicationUdpIncoming->GetRecordDelaymsDetailsTimeStampLogUs();
            const std::vector<uint64_t> pkt_size = sagApplicationUdpIncoming->GetRecordPktSizeBytes();
            
            double average_delay;
            if (pkt_delay.empty()) {
				average_delay = 0.0;
			}
			else{
				double sum = 0.0;
				for (const auto& delay : pkt_delay) {
						sum += delay;
				}
				average_delay = sum / pkt_delay.size();
			}

            std::vector<double> flow_rate;
            for(uint32_t i = 0; i < pkt_size.size() - 1; i++){
            	uint32_t j = i + 1;
            	flow_rate.push_back(byte_to_megabit(pkt_size[j] - pkt_size[i]) / ((record_timestamp[j] - record_timestamp[i])/1e6));
            }


            nlohmann::ordered_json jsonObject;
            jsonObject["name"] = "udp_" + std::to_string(info.GetBurstId());

            nlohmann::ordered_json jsonPathObject;
            jsonPathObject["max_hop_count"] = *max_it;
            jsonPathObject["min_hop_count"] = *min_it;
            jsonPathObject["maxdif_hop_count"] = path_hop_count_maxdif;
            jsonPathObject["path_change_times"] = path_change_number;
            jsonPathObject["path_hop_count"] = path_hop_count;
            jsonPathObject["path_hop_count_timestamp_us"] = route_timestamp;
            jsonObject["path_info"] = jsonPathObject;

			std::ofstream pathRecord(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(info.GetBurstId())+"/udp_" + std::to_string(info.GetBurstId())+"_path_log.json", std::ofstream::out);
			if (pathRecord.is_open()) {
				pathRecord << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			nlohmann::ordered_json jsonObject1;
			jsonObject1["name"] = "udp_" + std::to_string(info.GetBurstId());
            jsonObject1["average_delay_ms"] = average_delay;
			jsonObject1["delay_sample_us"] = pkt_delay;
			jsonObject1["time_stamp_us"] = record_timestamp;
			jsonObject1["max_delay_us"] = sagApplicationUdpIncoming->GetMaxDelayUs();
			jsonObject1["min_delay_us"] = sagApplicationUdpIncoming->GetMinDelayUs();
			std::ofstream pathRecord1(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(info.GetBurstId())+"/udp_" + std::to_string(info.GetBurstId())+"_delay_log.json", std::ofstream::out);
			if (pathRecord1.is_open()) {
				pathRecord1 << jsonObject1.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord1.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			nlohmann::ordered_json jsonObject2;
			jsonObject2["name"] = "udp_" + std::to_string(info.GetBurstId());
			jsonObject2["flow_rate_mbps"] = flow_rate;
			jsonObject2["time_stamp_us"] = record_timestamp;
			std::ofstream pathRecord2(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(info.GetBurstId())+"/udp_" + std::to_string(info.GetBurstId())+"_flow_rate_log.json", std::ofstream::out);
			if (pathRecord2.is_open()) {
				pathRecord2 << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord2.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			// Route Record
			nlohmann::ordered_json path_change;
            int64_t end_time_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns : info.GetStartTimeNs() + info.GetDurationNs();
            for(uint32_t p = 0; p < routes.size(); p++){
				std::vector<uint32_t> path = routes[p];
				std::string name = "udp_" + std::to_string(info.GetBurstId()) + "_path_" + std::to_string(p);

				nlohmann::ordered_json referencelist;
				for(uint32_t k = 0; k < path.size(); k++){
					referencelist.push_back(std::to_string(path[k])+"#position");
				}

				int64_t cur_time_us = route_timestamp[p];
				int64_t next_time_us;
				if(p == routes.size()-1){
					if(cur_time_us > end_time_ns/1e3){
						next_time_us = cur_time_us + 100000 < m_simulation_end_time_ns? cur_time_us + 100000: m_simulation_end_time_ns; // 100ms
					}
					else{
						next_time_us = end_time_ns/1e3;
					}
				}
				else{
					next_time_us = route_timestamp[p+1];
				}
				JulianDate simulation_s =  start_time_jd;
				JulianDate simulation_start =  start_time_jd + MicroSeconds(cur_time_us);
				JulianDate simulation_end =  start_time_jd + MicroSeconds(next_time_us);
				JulianDate simulation_d =  start_time_jd + NanoSeconds(m_simulation_end_time_ns);
				std::string simulation_start_string = simulation_start.ToStringiso();
				std::string simulation_end_string = simulation_end.ToStringiso();
				std::string simulation_s_string = simulation_s.ToStringiso();
				std::string simulation_d_string = simulation_d.ToStringiso();
				std::string temp_time = simulation_start_string + '/' +simulation_end_string;
				std::string temp_time_front = simulation_s_string + '/' +simulation_start_string;
				std::string temp_time_back = simulation_end_string + '/' +simulation_d_string;

				nlohmann::ordered_json showArray;
				nlohmann::ordered_json TempshowArray_temp = {
					{ "interval", temp_time },
					{ "boolean", true }
				};
				nlohmann::ordered_json TempshowArray_front = {
					{ "interval", temp_time_front },
					{ "boolean", false }
				};
				nlohmann::ordered_json TempshowArray_back = {
					{ "interval", temp_time_back },
					{ "boolean", false }
				};
				showArray.push_back(TempshowArray_front);
				showArray.push_back(TempshowArray_temp);
				showArray.push_back(TempshowArray_back);


				nlohmann::ordered_json jsonContent = {
						{"id", name},
						{"name", name},
						{"description", ""},
						{"polyline", {
							{"show", showArray},
							{"width", 3},
							{"zIndex", 99},
							{"material", {
								{"polylineOutline", {
									{"color", {
											{"rgba", {255, 0, 0, 255}}
									}},
									{"outlineColor", {
											{"rgba", {255, 0, 0, 255}}
									}},
									{"outlineWidth", 5}
								}}
							}},
							{"arcType", "NONE"},
							{"positions", {
								{"references", referencelist}
							}}
						}}
				};

				path_change.push_back(jsonContent);
			}

			std::ofstream pathCzml(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/udp_" + std::to_string(info.GetBurstId())+"/udp_" + std::to_string(info.GetBurstId())+"_czml.json", std::ofstream::out);
			if (pathCzml.is_open()) {
				pathCzml << path_change.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathCzml.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}
        }


        // Outgoing bursts
        std::cout << "  > Writing outgoing log files" << std::endl;
        nlohmann::ordered_json jsonUdpOutgoing;
        for (std::pair<SAGBurstInfoUdp, Ptr<SAGApplicationLayerUdp>> p : m_responsible_for_outgoing_bursts) {
            SAGBurstInfoUdp info = p.first;
            Ptr<SAGApplicationLayerUdp> sagApplicationUdpOutgoing = p.second;
            
            // Fetch data from the application
            //uint32_t complete_packet_size = 1500;
            //uint32_t max_payload_size_byte = sagApplicationUdpOutgoing->GetMaxPayloadSizeByte();
            uint64_t sent_counter = sagApplicationUdpOutgoing->GetSentCounterOf(info.GetBurstId());
            uint64_t sent_counter_size = sagApplicationUdpOutgoing->GetSentCounterSizeOf(info.GetBurstId());

            // Calculate outgoing rate
            int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
            //double rate_incl_headers_megabit_per_s = byte_to_megabit(sent_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
            //double rate_payload_only_megabit_per_s = byte_to_megabit(sent_counter * max_payload_size_byte) / nanosec_to_sec(effective_duration_ns);
            long unsigned int int_sent_incl_headers = sent_counter * 28 + sent_counter_size;
            long unsigned int int_sent_payload_only = sent_counter_size;
            double rate_incl_headers_megabit_per_s = byte_to_megabit(int_sent_incl_headers) / nanosec_to_sec(effective_duration_ns);
            double rate_payload_only_megabit_per_s = byte_to_megabit(int_sent_payload_only) / nanosec_to_sec(effective_duration_ns);

            // Write plain to the CSV
            fprintf(
                    file_outgoing_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
                    info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
                    info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, sent_counter,
					int_sent_incl_headers, int_sent_payload_only, info.GetMetadata().c_str()
            );

            // Write nicely formatted to the text
            char str_target_rate[100];
            sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
            char str_start_time[100];
            sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
            char str_duration_ms[100];
            sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
            char str_eff_rate_incl_headers[100];
            sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
            char str_eff_rate_payload_only[100];
            sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
            char str_sent_incl_headers[100];
            sprintf(str_sent_incl_headers, "%.2f Mbit", byte_to_megabit(int_sent_incl_headers));
            char str_sent_payload_only[100];
            sprintf(str_sent_payload_only, "%.2f Mbit", byte_to_megabit(int_sent_payload_only));
            fprintf(
                    file_outgoing_txt,
                    "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-16" PRIu64 "%-28s%-28s%s\n",
                    info.GetBurstId(),
                    info.GetFromNodeId(),
                    info.GetToNodeId(),
                    str_target_rate,
                    str_start_time,
                    str_duration_ms,
                    str_eff_rate_incl_headers,
                    str_eff_rate_payload_only,
                    sent_counter,
                    str_sent_incl_headers,
                    str_sent_payload_only,
                    info.GetMetadata().c_str()
            );


            nlohmann::ordered_json jsonObject;
            jsonObject["Applicartion ID"] = info.GetBurstId();
            jsonObject["From"] = info.GetFromNodeId();
            jsonObject["To"] = info.GetToNodeId();
            jsonObject["Target rate"] = str_target_rate;
            jsonObject["Start time"] = str_start_time;
            jsonObject["Duration"] = str_duration_ms;
            jsonObject["Outgoing rate (payload)"] = str_eff_rate_payload_only;
            jsonObject["Packets sent"] = sent_counter;
            jsonObject["Data sent (payload)"] = str_sent_payload_only;
            jsonObject["Metadata"] = info.GetMetadata().c_str();
            jsonUdpOutgoing.push_back(jsonObject);

        }
		std::ofstream udpOutgoingRecord(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_udp_flows_outgoing.json", std::ofstream::out);
		if (udpOutgoingRecord.is_open()) {
			udpOutgoingRecord << jsonUdpOutgoing.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			udpOutgoingRecord.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

        // Incoming bursts
        std::cout << "  > Writing incoming log files" << std::endl;
        nlohmann::ordered_json jsonUdpIncoming;
        for (std::pair<SAGBurstInfoUdp, Ptr<SAGApplicationLayerUdp>> p : m_responsible_for_incoming_bursts) {
            SAGBurstInfoUdp info = p.first;
            Ptr<SAGApplicationLayerUdp> sagApplicationUdpIncoming = p.second;

            // Fetch data from the application
            //uint32_t complete_packet_size = 1500;
            //uint32_t max_payload_size_byte = sagApplicationUdpIncoming->GetMaxPayloadSizeByte();
            uint64_t received_counter = sagApplicationUdpIncoming->GetReceivedCounterOf(info.GetBurstId());
            uint64_t received_counter_size = sagApplicationUdpIncoming->GetReceivedCounterSizeOf(info.GetBurstId());


            // Calculate incoming rate
            int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
//            double rate_incl_headers_megabit_per_s = byte_to_megabit(received_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
//            double rate_payload_only_megabit_per_s = byte_to_megabit(received_counter * max_payload_size_byte) / nanosec_to_sec(effective_duration_ns);
            long unsigned int int_received_incl_headers = received_counter * 28 + received_counter_size;
            long unsigned int int_received_payload_only = received_counter_size;
            double rate_incl_headers_megabit_per_s = byte_to_megabit(int_received_incl_headers) / nanosec_to_sec(effective_duration_ns);
            double rate_payload_only_megabit_per_s = byte_to_megabit(int_received_payload_only) / nanosec_to_sec(effective_duration_ns);


            // Write plain to the CSV
            fprintf(
                    file_incoming_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
                    info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
                    info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, received_counter,
					int_received_incl_headers, int_received_payload_only, info.GetMetadata().c_str()
            );

            // Write nicely formatted to the text
            char str_target_rate[100];
            sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
            char str_start_time[100];
            sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
            char str_duration_ms[100];
            sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
            char str_eff_rate_incl_headers[100];
            sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
            char str_eff_rate_payload_only[100];
            sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
            char str_received_incl_headers[100];
            sprintf(str_received_incl_headers, "%.2f Mbit", byte_to_megabit(int_received_incl_headers));
            char str_received_payload_only[100];
            sprintf(str_received_payload_only, "%.2f Mbit", byte_to_megabit(int_received_payload_only));
            fprintf(
                    file_incoming_txt,
                    "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-19" PRIu64 "%-28s%-28s%s\n",
                    info.GetBurstId(),
                    info.GetFromNodeId(),
                    info.GetToNodeId(),
                    str_target_rate,
                    str_start_time,
                    str_duration_ms,
                    str_eff_rate_incl_headers,
                    str_eff_rate_payload_only,
                    received_counter,
                    str_received_incl_headers,
                    str_received_payload_only,
                    info.GetMetadata().c_str()
            );

            nlohmann::ordered_json jsonObject;
            jsonObject["Applicartion ID"] = info.GetBurstId();
            jsonObject["From"] = info.GetFromNodeId();
            jsonObject["To"] = info.GetToNodeId();
            jsonObject["Target rate"] = str_target_rate;
            jsonObject["Start time"] = str_start_time;
            jsonObject["Duration"] = str_duration_ms;
            jsonObject["Incoming rate (payload)"] = str_eff_rate_payload_only;
            jsonObject["Packets received"] = received_counter;
            jsonObject["Data received (payload)"] = str_received_payload_only;
            jsonObject["Metadata"] = info.GetMetadata().c_str();
            jsonUdpIncoming.push_back(jsonObject);

        }
		std::ofstream udpIncomingRecord(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_udp_flows_incoming.json", std::ofstream::out);
		if (udpIncomingRecord.is_open()) {
			udpIncomingRecord << jsonUdpIncoming.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			udpIncomingRecord.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

        // Close files
        std::cout << "  > Closing SAG application log files:" << std::endl;
        fclose(file_outgoing_csv);
        std::cout << "    >> Closed: " << m_sag_bursts_outgoing_csv_filename << std::endl;
        fclose(file_outgoing_txt);
        std::cout << "    >> Closed: " << m_sag_bursts_outgoing_txt_filename << std::endl;
        fclose(file_incoming_csv);
        std::cout << "    >> Closed: " << m_sag_bursts_incoming_csv_filename << std::endl;
        fclose(file_incoming_txt);
        std::cout << "    >> Closed: " << m_sag_bursts_incoming_txt_filename << std::endl;


        // Register completion
        std::cout << "  > SAG application log files have been written" << std::endl;
        m_basicSimulation->RegisterTimestamp("Write application log files");


    }

    std::cout << std::endl;
}





/**
 * Read in the SAG application Udp schedule.
 *
 * @param filename                  File name of the sag_burst_schedule.csv
 * @param topology                  Topology
 * @param simulation_end_time_ns    Simulation end time (ns) : all SAG applicaitons must start less than this value
*/
std::vector<SAGBurstInfoUdp> read_sag_applicaton_schedule(const std::string& filename, Ptr<TopologySatelliteNetwork> topology, const int64_t simulation_end_time_ns)
{

    // Schedule to put in the data
    std::vector<SAGBurstInfoUdp> schedule;

    // Check that the file exists
    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }

    uint32_t gsIdStart = topology->GetNumSatellites(); //add application to groundstation
    //uint32_t gsIdStart = 0; //add application to sat

    // Open file
    std::ifstream schedule_file(filename);
    if (schedule_file) {

		json j;
		schedule_file >> j;
		jsonns::application_schedules udps = j.at("udp_flows");
		std::vector<jsonns::application_schedule> my_application_schedules_cur = udps.my_application_schedules;


		// Go over each line
		size_t line_counter = 0;
		int64_t prev_start_time_ns = 0;
		for(uint32_t i = 0; i < my_application_schedules_cur.size(); i++) {

			jsonns::application_schedule cur_application_schedule = my_application_schedules_cur[i];

			// Fill entry
			int64_t sag_application_id = parse_positive_int64(cur_application_schedule.flow_id);
			if (sag_application_id != (int64_t) line_counter) {
				throw std::invalid_argument(format_string("Application ID is not ascending by one each line (violation: %" PRId64 ")\n", sag_application_id));
			}
			int64_t from_node_id = gsIdStart + parse_positive_int64(cur_application_schedule.sender);
			int64_t to_node_id = gsIdStart + parse_positive_int64(cur_application_schedule.receiver);
			double target_rate_megabit_per_s = parse_positive_double(cur_application_schedule.target_flow_rate_mbps);
			int64_t start_time_ns = parse_positive_int64(cur_application_schedule.start_time_ns);
			int64_t duration_ns = parse_positive_int64(cur_application_schedule.duration_time_ns);
			std::string additional_parameters = remove_start_end_double_quote_if_present(cur_application_schedule.code_type);
			std::string metadata = remove_start_end_double_quote_if_present(cur_application_schedule.service_type);
			metadata.erase(std::remove_if(metadata.begin(),metadata.end(),::isspace),metadata.end());


			// Zero target rate
			if (target_rate_megabit_per_s == 0.0) {
				throw std::invalid_argument("Application target rate is zero.");
			}

			// Must be weakly ascending start time
			if (prev_start_time_ns > start_time_ns) {
				throw std::invalid_argument(format_string("Start time is not weakly ascending (on line with Application ID: %" PRId64 ", violation: %" PRId64 ")\n", sag_application_id, start_time_ns));
			}
			prev_start_time_ns = start_time_ns;

			// Check node IDs
			if (from_node_id == to_node_id) {
				throw std::invalid_argument(format_string("Application to itself at node ID: %" PRId64 ".", to_node_id));
			}

			// Check endpoint validity
			if (!topology->IsValidEndpoint(from_node_id)) {
				throw std::invalid_argument(format_string("Invalid from-endpoint for a schedule entry based on topology: %d", from_node_id));
			}
			if (!topology->IsValidEndpoint(to_node_id)) {
				throw std::invalid_argument(format_string("Invalid to-endpoint for a schedule entry based on topology: %d", to_node_id));
			}

			// Check start time
			if (start_time_ns >= simulation_end_time_ns) {
				throw std::invalid_argument(format_string(
						"Application %" PRId64 " has invalid start time %" PRId64 " >= %" PRId64 ".",
						sag_application_id, start_time_ns, simulation_end_time_ns
				));
			}

			// Put into schedule
			schedule.push_back(SAGBurstInfoUdp(sag_application_id, from_node_id, to_node_id, target_rate_megabit_per_s, start_time_ns, duration_ns, additional_parameters, metadata));

			// Next line
			line_counter++;

		}


        // Close file
        schedule_file.close();

    } else {
        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }

    return schedule;

}




}
