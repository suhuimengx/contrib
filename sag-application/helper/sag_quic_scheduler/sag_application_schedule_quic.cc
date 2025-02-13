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

 //Author: Mengy's:: Code

#include "sag_application_schedule_quic.h"

#include "ns3/cppjson2structure.hh"

namespace ns3 {

//Mengy's:: 这里面的Tcp对应的是Quic的流量
SAGApplicationSchedulerQuic::SAGApplicationSchedulerQuic(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
:SAGApplicationScheduler(basicSimulation, topology)
{
    printf("APPLICATION SCHEDULER QUIC\n");

    // Check if it is enabled explicitly
    //m_enabled = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sag_application_scheduler_tcp", "false"));
    //Mengy's::
    m_enabled = ReadApplicationEnable("quic_flow");
    if (!m_enabled) {
        std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

    } else {
        std::cout << "  > Quic flow scheduler is enabled" << std::endl;

        // Mengy's::Read logging
        ReadLoggingForApplicationIds("quic");

        // Mengy's:: Read schedule
        std::vector<SAGBurstInfoTcp> complete_schedule = read_quic_flow_schedule(
                m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_quic.json",
                m_topology,
                m_simulation_end_time_ns
        );

        // Check that the TCP flow IDs exist in the logging
        for (int64_t tcp_flow_id : m_enable_logging_for_sag_application_ids) {
            if ((size_t) tcp_flow_id >= complete_schedule.size()) {
                throw std::invalid_argument("Invalid Quic flow ID in sag_applicaiton_enable_logging_for_sag_application_quic_ids: " + std::to_string(tcp_flow_id));
            }
            if(m_system_id == 0){
            	//Mengy's::
    			mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/quic_" + std::to_string(tcp_flow_id));
            }
        }

        // Filter the schedule to only have applications starting at nodes which are part of this system
        if (m_enable_distributed) {
            std::vector<SAGBurstInfoTcp> filtered_schedule;
            for (SAGBurstInfoTcp &entry : complete_schedule) {
                if (m_distributed_node_system_id_assignment[entry.GetFromNodeId()] == m_system_id) {
                    filtered_schedule.push_back(entry);
                }
            }
            m_schedule = filtered_schedule;
        } else {
            m_schedule = complete_schedule;
        }

        // Schedule read
        printf("  > Read schedule (total flow start events: %lu)\n", m_schedule.size());
        m_basicSimulation->RegisterTimestamp("Read flow schedule");

        // Determine filenames
//        if (m_enable_distributed) {
//            m_flows_csv_filename =
//                    m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_tcp_flows.csv";
//            m_flows_txt_filename =
//                    m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_tcp_flows.txt";
//        } else {
//            m_flows_csv_filename = m_basicSimulation->GetLogsDir() + "/tcp_flows.csv";
//            m_flows_txt_filename = m_basicSimulation->GetLogsDir() + "/tcp_flows.txt";
//        }
        m_flows_csv_filename =
                m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_quic_flows.csv";
        m_flows_txt_filename =
                m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_quic_flows.txt";

        // Remove files if they are there
        remove_file_if_exists(m_flows_csv_filename);
        remove_file_if_exists(m_flows_txt_filename);
        printf("  > Removed previous flow log files if present\n");
        m_basicSimulation->RegisterTimestamp("Remove previous flow log files");


        // Install sink on endpoint node
        uint16_t my_port_num = 3026;
        uint16_t dst_port_num = 3026;

        std::cout << "  > Setting up applications on endpoint nodes" << std::endl;
        for (SAGBurstInfoTcp entry : complete_schedule) {
        	int64_t src = entry.GetFromNodeId();
        	int64_t dst = entry.GetToNodeId();

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[src] == m_system_id){
        		SAGApplicationHelperQuicSend source(
						m_basicSimulation,
						entry,
						dst_port_num,
						m_enable_logging_for_sag_application_ids.find(entry.GetTcpFlowId()) != m_enable_logging_for_sag_application_ids.end()
				);

				// Install it on the node and start it right now
				ApplicationContainer app = source.Install(m_nodes.Get(entry.GetFromNodeId()));
				app.Start(NanoSeconds(entry.GetStartTimeNs()));
				m_apps.push_back(app);

				// must be both set in sinks and senders
				app.Get(0)->GetObject<SAGApplicationLayer>()->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));
				app.Get(0)->GetObject<SAGApplicationLayer>()->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));

				app.Get(0)->GetObject<SAGApplicationLayerQuicSend>()->SetFlowEntry(entry);
                m_responsible_for_outgoing_bursts.push_back(std::make_pair(entry, app.Get(0)->GetObject<SAGApplicationLayerQuicSend>()));

        	}

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[dst] == m_system_id){
        		SAGApplicationHelperQuicSink sink(
        				m_basicSimulation,
						"ns3::QuicSocketFactory",
						InetSocketAddress(Ipv4Address::GetAny(), my_port_num),
						m_enable_logging_for_sag_application_ids.find(entry.GetTcpFlowId()) != m_enable_logging_for_sag_application_ids.end());
				ApplicationContainer app = sink.Install(m_nodes.Get(dst));
				app.Start(Seconds(0.0));

				// must be both set in sinks and senders
				app.Get(0)->GetObject<SAGApplicationLayer>()->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));
				app.Get(0)->GetObject<SAGApplicationLayer>()->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
                m_responsible_for_incoming_bursts.push_back(std::make_pair(entry, app.Get(0)->GetObject<SAGApplicationLayerQuicSink>()));

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
        std::cout << "  > Setting up traffic Quic flow starter" << std::endl;
        m_basicSimulation->RegisterTimestamp("Setup traffic Quic flow starter");


    }

    std::cout << std::endl;
}

void SAGApplicationSchedulerQuic::WriteResults() {
    std::cout << "STORE QUIC FLOW RESULTS" << std::endl;

    // Check if it is enabled explicitly
    if (!m_enabled) {
        std::cout << "  > Not enabled, so no QUIC flow results are written" << std::endl;

    } else {

        // Open files
        std::cout << "  > Opening QUIC flow log files:" << std::endl;
        FILE* file_csv = fopen(m_flows_csv_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_flows_csv_filename << std::endl;
        FILE* file_txt = fopen(m_flows_txt_filename.c_str(), "w+");
        std::cout << "    >> Opened: " << m_flows_txt_filename << std::endl;

        // Header
        std::cout << "  > Writing quic_flows.txt header" << std::endl;
        fprintf(
                file_txt, "%-16s%-10s%-10s%-16s%-18s%-18s%-16s%-16s%-13s%-16s%-14s%s\n",
                "QUIC Flow ID", "Source", "Target", "Size (MbpS)", "Start time (ns)",
                "End time (ns)", "Duration", "Sent", "Progress", "Avg. rate", "Finished?", "Metadata"
        );

        // Go over the schedule, write each flow's result
        std::cout << "  > Writing log files line-by-line" << std::endl;
        std::cout << "  > Total QUIC flow log entries to write... " << m_apps.size() << std::endl;
        uint32_t app_idx = 0;
        nlohmann::ordered_json jsonTcp;
        for (SAGBurstInfoTcp& entry : m_schedule) {

            // Retrieve application
            Ptr<SAGApplicationLayerQuicSend> flowSendApp = m_apps.at(app_idx).Get(0)->GetObject<SAGApplicationLayerQuicSend>();

            if(flowSendApp == nullptr)  // debug later
            {   
                std::cout << "  > Quic Flow Send Ptr at "<< app_idx <<" is not available!!" << std::endl;
                app_idx +=1 ;
                continue;  
            }

            // Finalize the detailed logs (if they are enabled)
            flowSendApp->FinalizeDetailedLogs();

            // Retrieve statistics
            bool is_completed = flowSendApp->IsCompleted();
            bool is_conn_failed = flowSendApp->IsConnFailed();
            bool is_closed_err = flowSendApp->IsClosedByError();
            bool is_closed_normal = flowSendApp->IsClosedNormally();
            int64_t sent_byte = flowSendApp->GetAckedBytes();
            int64_t sent_byte_all = flowSendApp->GetTotalBytes();
            int64_t fct_ns;
            if (is_completed) {
                fct_ns = flowSendApp->GetCompletionTimeNs() - entry.GetStartTimeNs();
            } else {
                fct_ns = m_simulation_end_time_ns - entry.GetStartTimeNs();
            }
            std::string finished_state;
            if (is_completed) {
                finished_state = "YES";
            } else if (is_conn_failed) {
                finished_state = "NO_CONN_FAIL";
            } else if (is_closed_normal) {
                finished_state = "NO_BAD_CLOSE";
            } else if (is_closed_err) {
                finished_state = "NO_ERR_CLOSE";
            } else {
                finished_state = "NO_ONGOING";
            }

            // Write plain to the csv
            fprintf(
                    file_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%s,%s\n",
                    entry.GetTcpFlowId(), entry.GetFromNodeId(), entry.GetToNodeId(), entry.GetTargetRateMegabitPerSec(), entry.GetStartTimeNs(),
                    entry.GetStartTimeNs() + fct_ns, fct_ns, sent_byte, finished_state.c_str(), entry.GetMetadata().c_str()
            );

            // Write nicely formatted to the text
            char str_size_megabit[100];
            sprintf(str_size_megabit, "%.2f Mbit/s", entry.GetTargetRateMegabitPerSec());
            char str_duration_ms[100];
            sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(fct_ns));
            char str_sent_megabit[100];
            sprintf(str_sent_megabit, "%.2f Mbit", byte_to_megabit(sent_byte));
            char str_progress_perc[100];
            sprintf(str_progress_perc, "%.1f%%", ((double) sent_byte) / ((double) sent_byte_all) * 100.0);
            char str_avg_rate_megabit_per_s[100];
            sprintf(str_avg_rate_megabit_per_s, "%.2f Mbit/s", byte_to_megabit(sent_byte) / nanosec_to_sec(fct_ns));
            fprintf(
                    file_txt, "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-16s%-18" PRId64 "%-18" PRId64 "%-16s%-16s%-13s%-16s%-14s%s\n",
                    entry.GetTcpFlowId(), entry.GetFromNodeId(), entry.GetToNodeId(), str_size_megabit, entry.GetStartTimeNs(),
                    entry.GetStartTimeNs() + fct_ns, str_duration_ms, str_sent_megabit, str_progress_perc, str_avg_rate_megabit_per_s,
                    finished_state.c_str(), entry.GetMetadata().c_str()
            );

            // Move on application index
            app_idx += 1;

            char str_start_time[100];
            sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(entry.GetStartTimeNs()));
            char str_end_time[100];
            sprintf(str_end_time, "%.2f ms", nanosec_to_millisec(entry.GetStartTimeNs() + fct_ns));
            nlohmann::ordered_json jsonObject;
            jsonObject["Applicartion ID"] = entry.GetTcpFlowId();
            jsonObject["From"] = entry.GetFromNodeId();
            jsonObject["To"] = entry.GetToNodeId();
            jsonObject["Target rate"] = str_size_megabit;
            jsonObject["Start time"] = str_start_time;
            jsonObject["End time"] = str_end_time;
            jsonObject["Duration"] = str_duration_ms;
            jsonObject["Data sent (payload)"] = str_sent_megabit;
            jsonObject["Progress"] = str_progress_perc;
            jsonObject["Avg. rate"] = str_avg_rate_megabit_per_s;
            jsonObject["Finished?"] = finished_state.c_str();
            jsonObject["Metadata"] = entry.GetMetadata().c_str();
            jsonTcp.push_back(jsonObject);

        }
		std::ofstream tcpRecord(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_quic_flows.json", std::ofstream::out);
		if (tcpRecord.is_open()) {
			tcpRecord << jsonTcp.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			tcpRecord.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}
        // Close files
        std::cout << "  > Closing QUIC flow log files:" << std::endl;
        fclose(file_csv);
        std::cout << "    >> Closed: " << m_flows_csv_filename << std::endl;
        fclose(file_txt);
        std::cout << "    >> Closed: " << m_flows_txt_filename << std::endl;






		JulianDate start_time_jd = m_nodes.Get(0)->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
		//mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics");
		for (std::pair<SAGBurstInfoTcp, Ptr<SAGApplicationLayerQuicSink>> p : m_responsible_for_incoming_bursts) {

			SAGBurstInfoTcp info = p.first;
			Ptr<SAGApplicationLayerQuicSink> sagApplicationFtpIncoming = p.second;
			if(m_enable_logging_for_sag_application_ids.find(info.GetTcpFlowId()) == m_enable_logging_for_sag_application_ids.end()){
				continue;
			}
			//mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/tcp_" + std::to_string(info.GetTcpFlowId()));

			// Flow Details
			const std::vector<std::vector<uint32_t>> routes = sagApplicationFtpIncoming->GetRecordRouteDetailsLog();
            if(routes.size() == 0){
            	continue;
            }
			const std::vector<int64_t> route_timestamp =  sagApplicationFtpIncoming->GetRecordRouteDetailsTimeStampLogUs();
			uint32_t path_change_number = routes.size() - 1;
			std::vector<uint32_t> path_hop_count;
			for(auto path: routes){
				path_hop_count.push_back(path.size());
			}
			auto max_it = std::max_element(path_hop_count.begin(), path_hop_count.end());
			auto min_it = std::min_element(path_hop_count.begin(), path_hop_count.end());
			uint32_t path_hop_count_maxdif = *max_it - *min_it;
			const std::vector<int64_t> record_timestamp =  sagApplicationFtpIncoming->GetRecordTimeStampLogUs();


			nlohmann::ordered_json jsonObject;
			jsonObject["name"] = "quic_" + std::to_string(info.GetTcpFlowId());

			nlohmann::ordered_json jsonPathObject;
			jsonPathObject["max_hop_count"] = *max_it;
			jsonPathObject["min_hop_count"] = *min_it;
			jsonPathObject["maxdif_hop_count"] = path_hop_count_maxdif;
			jsonPathObject["path_change_times"] = path_change_number;
            jsonPathObject["path_hop_count"] = path_hop_count;
            jsonPathObject["path_hop_count_timestamp_us"] = route_timestamp;
			jsonObject["path_info"] = jsonPathObject;

			std::ofstream pathRecord(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/quic_" + std::to_string(info.GetTcpFlowId())+"/quic_" + std::to_string(info.GetTcpFlowId())+"_path_log.json", std::ofstream::out);
			if (pathRecord.is_open()) {
				pathRecord << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}


			// Route Record
			nlohmann::ordered_json path_change;
			int64_t end_time_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns : info.GetStartTimeNs() + info.GetDurationNs();
			//int64_t end_time_us = record_timestamp[record_timestamp.size()-1];
			for(uint32_t p = 0; p < routes.size(); p++){
				std::vector<uint32_t> path = routes[p];
				std::string name = "quic_" + std::to_string(info.GetTcpFlowId()) + "_path_" + std::to_string(p);

				nlohmann::ordered_json referencelist;
				for(uint32_t k = 0; k < path.size(); k++){
					referencelist.push_back(std::to_string(path[k])+"#position");
				}

				int64_t cur_time_us = route_timestamp[p];
				int64_t next_time_us;
				if(p == routes.size()-1){
					if(cur_time_us >= end_time_ns/1e3){
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

			std::ofstream pathCzml(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/quic_" + std::to_string(info.GetTcpFlowId())+"/quic_" + std::to_string(info.GetTcpFlowId())+"_czml.json", std::ofstream::out);
			if (pathCzml.is_open()) {
				pathCzml << path_change.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathCzml.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}
		}




		for (std::pair<SAGBurstInfoTcp, Ptr<SAGApplicationLayerQuicSend>> p : m_responsible_for_outgoing_bursts) {
			SAGBurstInfoTcp info = p.first;
			Ptr<SAGApplicationLayerQuicSend> sagApplicationFtpIncoming = p.second;
			if(m_enable_logging_for_sag_application_ids.find(info.GetTcpFlowId()) == m_enable_logging_for_sag_application_ids.end()){
				continue;
			}

			const std::vector<int64_t> record_timestamp =  sagApplicationFtpIncoming->GetRecordTimeStampLogUs();
			const std::vector<int64_t> record_process_timestamp =  sagApplicationFtpIncoming->GetRecordProcessTimeStampLogUs();
			const std::vector<double> pkt_delay =  sagApplicationFtpIncoming->GetRecordDelaymsDetailsTimeStampLogUs();
			const std::vector<uint64_t> pkt_size = sagApplicationFtpIncoming->GetRecordPktSizeBytes();
			std::vector<double> flow_rate;
			for(uint32_t i = 0; i < pkt_size.size() - 1; i++){
				uint32_t j = i + 1;
				flow_rate.push_back(byte_to_megabit(pkt_size[j] - pkt_size[i]) / ((record_process_timestamp[j] - record_process_timestamp[i])/1e6));
			}

			//std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!"<<m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/ftp_" + std::to_string(info.GetFtpFlowId())+"/ftp_" + std::to_string(info.GetFtpFlowId())+"_delay_log.json"<<std::endl;

			nlohmann::ordered_json jsonObject1;
			jsonObject1["name"] = "quic_" + std::to_string(info.GetTcpFlowId());
			jsonObject1["delay_sample_us"] = pkt_delay;
			jsonObject1["time_stamp_us"] = record_timestamp;
			std::ofstream pathRecord1(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/quic_" + std::to_string(info.GetTcpFlowId())+"/quic_" + std::to_string(info.GetTcpFlowId())+"_delay_log.json", std::ofstream::out);
			if (pathRecord1.is_open()) {
				pathRecord1 << jsonObject1.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord1.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			nlohmann::ordered_json jsonObject2;
			jsonObject2["name"] = "quic_" + std::to_string(info.GetTcpFlowId());
			jsonObject2["flow_rate_mbps"] = flow_rate;
			jsonObject2["time_stamp_us"] = record_process_timestamp;
			std::ofstream pathRecord2(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/quic_" + std::to_string(info.GetTcpFlowId())+"/quic_" + std::to_string(info.GetTcpFlowId())+"_flow_rate_log.json", std::ofstream::out);
			if (pathRecord2.is_open()) {
				pathRecord2 << jsonObject2.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord2.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

//			std::ofstream pathRecord1(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/tcp_" + std::to_string(info.GetTcpFlowId())+"/tcp_" + std::to_string(info.GetTcpFlowId())+"_delay_log.json", std::ofstream::out);
//			if (pathRecord1.is_open()) {
//				pathRecord1 << jsonObject1.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//				pathRecord1.close();
//				//std::cout << "JSON file created successfully." << std::endl;
//			} else {
//				std::cout << "Failed to create JSON file." << std::endl;
//			}

		}





        // Register completion
        std::cout << "  > QUIC flow log files have been written" << std::endl;
        m_basicSimulation->RegisterTimestamp("Write QUIC flow log files");

    }

    std::cout << std::endl;
}





/**
 * Read in the SAG application Tcp schedule.
 *
 * @param filename                  File name of the schedule.csv
 * @param topology                  Topology
 * @param simulation_end_time_ns    Simulation end time (ns) : all flows must start less than this value
*/
std::vector<SAGBurstInfoTcp> read_quic_flow_schedule(const std::string& filename, Ptr<TopologySatelliteNetwork> topology, const int64_t simulation_end_time_ns) {

//    // Schedule to put in the data
//    std::vector<SAGBurstInfoTcp> schedule;
//
//    // Check that the file exists
//    if (!file_exists(filename)) {
//        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
//    }
//
//    uint32_t gsIdStart = topology->GetNumSatellites();
//    //uint32_t gsIdStart = 0;
//
//    // Open file
//    std::string line;
//    std::ifstream schedule_file(filename);
//    if (schedule_file) {
//
//        // Go over each line
//        size_t line_counter = 0;
//        int64_t prev_start_time_ns = 0;
//        while (getline(schedule_file, line)) {
//
//            // Split on ,
//            std::vector<std::string> comma_split = split_string(line, ",", 8);
//
//            // Fill entry
//            int64_t tcp_flow_id = parse_positive_int64(comma_split[0]);
//            if (tcp_flow_id != (int64_t) line_counter) {
//                throw std::invalid_argument(format_string("TCP flow ID is not ascending by one each line (violation: %" PRId64 ")\n", tcp_flow_id));
//            }
//            int64_t from_node_id = gsIdStart + parse_positive_int64(comma_split[1]);
//            int64_t to_node_id = gsIdStart + parse_positive_int64(comma_split[2]);
//            double size_byte = parse_positive_double(comma_split[3]);
//            int64_t start_time_ns = parse_positive_int64(comma_split[4]);
//            int64_t duration_ns = parse_positive_int64(comma_split[5]);
//            std::string additional_parameters = comma_split[6];
//            std::string metadata = comma_split[7];
//            metadata.erase(std::remove_if(metadata.begin(),metadata.end(),::isspace),metadata.end());
//
//            // Must be weakly ascending start time
//            if (prev_start_time_ns > start_time_ns) {
//                throw std::invalid_argument(format_string("Start time is not weakly ascending (on line with TCP flow ID: %" PRId64 ", violation: %" PRId64 ")\n", tcp_flow_id, start_time_ns));
//            }
//            prev_start_time_ns = start_time_ns;
//
//            // Check node IDs
//            if (from_node_id == to_node_id) {
//                throw std::invalid_argument(format_string("TCP flow to itself at node ID: %" PRId64 ".", to_node_id));
//            }
//
//            // Check endpoint validity
//            if (!topology->IsValidEndpoint(from_node_id)) {
//                throw std::invalid_argument(format_string("Invalid from-endpoint for a schedule entry based on topology: %d", from_node_id));
//            }
//            if (!topology->IsValidEndpoint(to_node_id)) {
//                throw std::invalid_argument(format_string("Invalid to-endpoint for a schedule entry based on topology: %d", to_node_id));
//            }
//
//            // Check start time
//            if (start_time_ns >= simulation_end_time_ns) {
//                throw std::invalid_argument(format_string(
//                        "TCP flow %" PRId64 " has invalid start time %" PRId64 " >= %" PRId64 ".",
//                        tcp_flow_id, start_time_ns, simulation_end_time_ns
//                ));
//            }
//
//            // Put into schedule
//            schedule.push_back(SAGBurstInfoTcp(tcp_flow_id, from_node_id, to_node_id, size_byte, start_time_ns, duration_ns, additional_parameters, metadata));
//
//            // Next line
//            line_counter++;
//
//        }
//
//        // Close file
//        schedule_file.close();
//
//    } else {
//        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
//    }
//
//    return schedule;


    // Schedule to put in the data
    std::vector<SAGBurstInfoTcp> schedule;

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
		jsonns::application_schedules tcps = j.at("quic_flows");
		std::vector<jsonns::application_schedule> my_application_schedules_cur = tcps.my_application_schedules;


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
			schedule.push_back(SAGBurstInfoTcp(sag_application_id, from_node_id, to_node_id, target_rate_megabit_per_s, start_time_ns, duration_ns, additional_parameters, metadata));

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
