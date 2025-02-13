#include "ns3/sag_application_schedule_rtp.h"
#include "ns3/sag_application_layer_rtp_sender.h"
#include "ns3/sag_application_layer_rtp_receiver.h"
#include "ns3/cppjson2structure.hh"

namespace ns3 {

SAGApplicationSchedulerRtp::SAGApplicationSchedulerRtp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
:SAGApplicationScheduler(basicSimulation, topology)
{
    printf("APPLICATION SCHEDULER RTP\n");

    // Check if it is enabled explicitly
    //m_enabled = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sag_application_scheduler_rtp", "false"));
    m_enabled = ReadApplicationEnable("rtp_flow");
    if (!m_enabled) {
        std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

    } else {
    	std::cout << "  > SAG applicaiton scheduler is enabled" << std::endl;

        ReadLoggingForApplicationIds("rtp");

        // Read schedule
        std::vector<SAGBurstInfoRtp> complete_schedule = read_sag_applicaton_schedule_rtp(
                m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_rtp.json",
                m_topology,
                m_simulation_end_time_ns
        );


        // Filter the schedule to only have applications starting at nodes which are part of this system
        if (m_enable_distributed) {
            std::vector<SAGBurstInfoRtp> filtered_schedule;
            for (SAGBurstInfoRtp &entry : complete_schedule) {
                if (m_distributed_node_system_id_assignment[entry.GetFromNodeId()] == m_system_id) {
                    filtered_schedule.push_back(entry);
                }
            }
            m_schedule = filtered_schedule;
        } else {
            m_schedule = complete_schedule;
        }

        // Check that the SAG applicaiton IDs exist in the logging
        if(!m_enable_logging_for_sag_application_ids.empty()){
			for (int64_t sag_application_id : m_enable_logging_for_sag_application_ids) {
				if ((size_t) sag_application_id >= complete_schedule.size()) {
					throw std::invalid_argument("Invalid SAG burst ID in sag_applicaiton_enable_logging_for_sag_application_rtp_ids: " + std::to_string(sag_application_id));
				}
				if(m_system_id == 0){
		    		mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(sag_application_id));
				}
			}
        }

        // Schedule read
        printf("  > Read schedule (total SAG applications: %lu)\n", m_schedule.size());
        m_basicSimulation->RegisterTimestamp("Read SAG application schedule");

        // Determine filenames
//        if (m_enable_distributed) {
////            m_sag_bursts_outgoing_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_sag_bursts_outgoing.csv";
//            m_sag_bursts_outgoing_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_outgoing.txt";
////            m_sag_bursts_incoming_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_sag_bursts_incoming.csv";
//            m_sag_bursts_incoming_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_incoming.txt";
//        } else {
////            m_sag_bursts_outgoing_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/sag_bursts_outgoing.csv";
//            m_sag_bursts_outgoing_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/rtp_flows_outgoing.txt";
////            m_sag_bursts_incoming_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/sag_bursts_incoming.csv";
//            m_sag_bursts_incoming_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/rtp_flows_incoming.txt";
//        }

//            m_sag_bursts_outgoing_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_sag_bursts_outgoing.csv";
		m_sag_bursts_outgoing_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_outgoing.txt";
//            m_sag_bursts_incoming_csv_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_sag_bursts_incoming.csv";
		m_sag_bursts_incoming_txt_filename_rtp = m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_incoming.txt";

        // Remove files if they are there
//        remove_file_if_exists(m_sag_bursts_outgoing_csv_filename_rtp);
        remove_file_if_exists(m_sag_bursts_outgoing_txt_filename_rtp);
//        remove_file_if_exists(m_sag_bursts_incoming_csv_filename_rtp);
        remove_file_if_exists(m_sag_bursts_incoming_txt_filename_rtp);
        printf("  > Removed previous flow log files if present\n");
        m_basicSimulation->RegisterTimestamp("Remove previous flow log files");






        // Install sink on endpoint node
        uint16_t my_port_num = 5026;
        uint16_t dst_port_num = 5026;

        std::cout << "  > Setting up applications on endpoint nodes" << std::endl;
        for (SAGBurstInfoRtp entry : complete_schedule) {
        	int64_t src = entry.GetFromNodeId();
        	int64_t dst = entry.GetToNodeId();

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[src] == m_system_id){
        		SAGApplicationHelperRtpSender source(m_basicSimulation, entry, dst_port_num);

        		ApplicationContainer app_send = source.Install(m_nodes.Get(src));
				app_send.Start(NanoSeconds(entry.GetStartTimeNs()));
				m_apps.push_back(app_send);
				Ptr<SAGApplicationLayerRTPSender> sagApplicationLayerRtpsend = app_send.Get(0)->GetObject<SAGApplicationLayerRTPSender>();

				// Register all bursts being sent from there
				sagApplicationLayerRtpsend->RegisterOutgoingBurst(
						entry,
						m_nodes.Get(entry.GetToNodeId()),
						dst_port_num,
						m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
				);

				m_responsible_for_outgoing_bursts_rtp.push_back(std::make_pair(entry, sagApplicationLayerRtpsend));

				// must be both set in sinks and senders
                sagApplicationLayerRtpsend->SetBasicSimuAttr(m_basicSimulation);
                sagApplicationLayerRtpsend->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
                sagApplicationLayerRtpsend->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));

        	}

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[dst] == m_system_id){
       	        //std::cout << "  > Setting up SAG applications on destination nodes: " << entry.GetToNodeId()  << " port: " << port_num  << std::endl; //<< m_nodes.Get(endpoint)
                // Setup the application
                SAGApplicationHelperRtpReceiver sink(m_basicSimulation, my_port_num);
                ApplicationContainer app_receive = sink.Install(m_nodes.Get(dst));
                app_receive.Start(Seconds(0.0));
                m_apps.push_back(app_receive);
                Ptr<SAGApplicationLayerRTPReceiver> sagApplicationLayerRtpreceive = app_receive.Get(0)->GetObject<SAGApplicationLayerRTPReceiver>();


                // Register all bursts being received
                sagApplicationLayerRtpreceive->RegisterIncomingBurst(
                		entry,
						m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
						);
                m_responsible_for_incoming_bursts_rtp.push_back(std::make_pair(entry, sagApplicationLayerRtpreceive));

				// must be both set in sinks and senders
                sagApplicationLayerRtpreceive->SetBasicSimuAttr(m_basicSimulation);
                sagApplicationLayerRtpreceive->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
                sagApplicationLayerRtpreceive->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));

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
        std::cout << "  > Setting up traffic RTP flow starter" << std::endl;
        m_basicSimulation->RegisterTimestamp("Setup applications on endpoint nodes");




//        // Install app on each endpoint node
//        std::cout << "  > Setting up SAG applications on all endpoint nodes" << std::endl;
//        for (int64_t endpoint : m_topology->GetEndpoints()) {
//            if (!m_enable_distributed || m_distributed_node_system_id_assignment[endpoint] == m_system_id) {
//                uint16_t port_num = 1026;
//                for (SAGBurstInfoRtp entry : m_schedule) {
//                    if (entry.GetFromNodeId() == endpoint) {
//                        //std::cout << "  > Setting up SAG applications on source nodes: " << entry.GetFromNodeId() << " port: " << port_num << std::endl;
//                        // Setup the application
//                        SAGApplicationHelperRtpSender sagApplicationHelperRtpsend(m_basicSimulation, port_num);
//                        ApplicationContainer app_send = sagApplicationHelperRtpsend.Install(m_nodes.Get(endpoint));
//                        app_send.Start(Seconds(0.0));
//                        m_apps.push_back(app_send);
//                        Ptr<SAGApplicationLayerRTPSender> sagApplicationLayerRtpsend = app_send.Get(0)->GetObject<SAGApplicationLayerRTPSender>();
//
//                        // Register all bursts being sent from there
//                        sagApplicationLayerRtpsend->RegisterOutgoingBurst(
//                        		entry,
//                        		m_nodes.Get(entry.GetToNodeId()),
//                        		port_num,
//                                m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
//                        );
//                        //m_responsible_for_outgoing_bursts_rtp.push_back(std::make_pair(entry, sagApplicationLayerRtpsend));
//                    }
//               		if (entry.GetToNodeId() == endpoint) {
//               	        //std::cout << "  > Setting up SAG applications on destination nodes: " << entry.GetToNodeId()  << " port: " << port_num  << std::endl; //<< m_nodes.Get(endpoint)
//                        // Setup the application
//                        SAGApplicationHelperRtpReceiver sagApplicationHelperRtpreceive(m_basicSimulation, port_num);
//                        ApplicationContainer app_receive = sagApplicationHelperRtpreceive.Install(m_nodes.Get(endpoint));
//                        app_receive.Start(Seconds(0.0));
//                        m_apps.push_back(app_receive);
//                        Ptr<SAGApplicationLayerRTPReceiver> sagApplicationLayerRtpreceive = app_receive.Get(0)->GetObject<SAGApplicationLayerRTPReceiver>();
//
//
//                        // Register all bursts being received
//                        sagApplicationLayerRtpreceive->RegisterIncomingBurst(
//                        		entry,
//								m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()
//								);
//                        m_responsible_for_incoming_bursts_rtp.push_back(std::make_pair(entry, sagApplicationLayerRtpreceive));
//               		}
//                    port_num++;
//                }
//            }
//        }

    }
    std::cout << std::endl;
}

void SAGApplicationSchedulerRtp::WriteResults()
{
    std::cout << "STORE RTP FLOW RESULTS" << std::endl;

    // Check if it is enabled explicitly
    if (!m_enabled) {
        std::cout << "  > Not enabled, so no RTP flow results are written" << std::endl;

    } else {

//        // Open files
//        std::cout << "  > Opening RTP flow log files:" << std::endl;
//        FILE* file_csv = fopen(m_flows_csv_filename.c_str(), "w+");
//        std::cout << "    >> Opened: " << m_flows_csv_filename << std::endl;
//        FILE* file_txt = fopen(m_flows_txt_filename.c_str(), "w+");
//        std::cout << "    >> Opened: " << m_flows_txt_filename << std::endl;

		// Open files
		std::cout << "  > Opening RTP flow log files:" << std::endl;
//		FILE* file_outgoing_csv = fopen(m_sag_bursts_outgoing_csv_filename_rtp.c_str(), "w+");
//		std::cout << "    >> Opened: " << m_sag_bursts_outgoing_csv_filename_rtp << std::endl;
		FILE* file_outgoing_txt = fopen(m_sag_bursts_outgoing_txt_filename_rtp.c_str(), "w+");
		std::cout << "    >> Opened: " << m_sag_bursts_outgoing_txt_filename_rtp << std::endl;
//		FILE* file_incoming_csv = fopen(m_sag_bursts_incoming_csv_filename_rtp.c_str(), "w+");
//		std::cout << "    >> Opened: " << m_sag_bursts_incoming_csv_filename_rtp << std::endl;
		FILE* file_incoming_txt = fopen(m_sag_bursts_incoming_txt_filename_rtp.c_str(), "w+");
		std::cout << "    >> Opened: " << m_sag_bursts_incoming_txt_filename_rtp << std::endl;

		// Header
		std::cout << "  > Writing RTP flow headers" << std::endl;

		fprintf(
				file_outgoing_txt, "%-16s%-10s%-10s%-16s%-16s%-28s%-19s%-28s%s\n",
				"Applicartion ID", "From", "To", "Start time", "Duration",
				"Outgoing rate (payload)", "Packets sent",
				"Data sent (payload)", "Metadata"
		);
		fprintf(
				file_incoming_txt, "%-16s%-10s%-10s%-16s%-16s%-28s%-19s%-28s%s\n",
				"Applicartion ID", "From", "To", "Start time", "Duration",
				"Incoming rate (payload)", "Packets received",
				"Data received (payload)", "Metadata"
		);


		// Sort ascending to preserve SAG applicartion schedule order
		struct ascending_paired_sag_application_id_key_send
		{
			inline bool operator() (const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>>& a, const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>>& b)
			{
				return (a.first.GetBurstId() < b.first.GetBurstId());
			}
		};
		struct ascending_paired_sag_application_id_key_receive
		{
			inline bool operator() (const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>>& a, const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>>& b)
			{
				return (a.first.GetBurstId() < b.first.GetBurstId());
			}
		};
		std::sort(m_responsible_for_outgoing_bursts_rtp.begin(), m_responsible_for_outgoing_bursts_rtp.end(), ascending_paired_sag_application_id_key_send());
		std::sort(m_responsible_for_incoming_bursts_rtp.begin(), m_responsible_for_incoming_bursts_rtp.end(), ascending_paired_sag_application_id_key_receive());





        JulianDate start_time_jd = m_nodes.Get(0)->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
        for (std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>> p : m_responsible_for_incoming_bursts_rtp) {

        	SAGBurstInfoRtp info = p.first;
            Ptr<SAGApplicationLayerRTPReceiver> sagApplicationUdpIncoming = p.second;
            if(m_enable_logging_for_sag_application_ids.find(info.GetBurstId()) == m_enable_logging_for_sag_application_ids.end()){
            	continue;
            }
    		//mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(info.GetBurstId()));

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
            std::vector<double> flow_rate;
            for(uint32_t i = 0; i < pkt_size.size() - 1; i++){
            	uint32_t j = i + 1;
            	flow_rate.push_back(byte_to_megabit(pkt_size[j] - pkt_size[i]) / ((record_timestamp[j] - record_timestamp[i])/1e6));
            }


            nlohmann::ordered_json jsonObject;
            jsonObject["name"] = "rtp_" + std::to_string(info.GetBurstId());

            nlohmann::ordered_json jsonPathObject;
            jsonPathObject["max_hop_count"] = *max_it;
            jsonPathObject["min_hop_count"] = *min_it;
            jsonPathObject["maxdif_hop_count"] = path_hop_count_maxdif;
            jsonPathObject["path_change_times"] = path_change_number;
            jsonPathObject["path_hop_count"] = path_hop_count;
            jsonPathObject["path_hop_count_timestamp_us"] = route_timestamp;
            jsonObject["path_info"] = jsonPathObject;

			std::ofstream pathRecord(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(info.GetBurstId())+"/rtp_" + std::to_string(info.GetBurstId())+"_path_log.json", std::ofstream::out);
			if (pathRecord.is_open()) {
				pathRecord << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			nlohmann::ordered_json jsonObject1;
			jsonObject1["name"] = "rtp_" + std::to_string(info.GetBurstId());
			jsonObject1["delay_sample_us"] = pkt_delay;
			jsonObject1["time_stamp_us"] = record_timestamp;
			std::ofstream pathRecord1(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(info.GetBurstId())+"/rtp_" + std::to_string(info.GetBurstId())+"_delay_log.json", std::ofstream::out);
			if (pathRecord1.is_open()) {
				pathRecord1 << jsonObject1.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				pathRecord1.close();
				//std::cout << "JSON file created successfully." << std::endl;
			} else {
				std::cout << "Failed to create JSON file." << std::endl;
			}

			nlohmann::ordered_json jsonObject2;
			jsonObject2["name"] = "rtp_" + std::to_string(info.GetBurstId());
			jsonObject2["flow_rate_mbps"] = flow_rate;
			jsonObject2["time_stamp_us"] = record_timestamp;
			std::ofstream pathRecord2(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(info.GetBurstId())+"/rtp_" + std::to_string(info.GetBurstId())+"_flow_rate_log.json", std::ofstream::out);
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
				std::string name = "rtp_" + std::to_string(info.GetBurstId()) + "_path_" + std::to_string(p);

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

			std::ofstream pathCzml(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/rtp_" + std::to_string(info.GetBurstId())+"/rtp_" + std::to_string(info.GetBurstId())+"_czml.json", std::ofstream::out);
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
        nlohmann::ordered_json jsonRtpOutgoing;
		for (std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>> p : m_responsible_for_outgoing_bursts_rtp) {
			SAGBurstInfoRtp info = p.first;
			Ptr<SAGApplicationLayerRTPSender> sagApplicationRtpOutgoing = p.second;

			// Fetch data from the application
			uint64_t send_counter = sagApplicationRtpOutgoing->GetSentCounterOf(info.GetBurstId());
			uint64_t send_size_counter = sagApplicationRtpOutgoing->GetSentSizeCounterOf(info.GetBurstId());

			// Calculate incoming rate
			int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
			//double rate_incl_headers_megabit_per_s = byte_to_megabit(received_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
			double rate_payload_only_megabit_per_s = byte_to_megabit(send_size_counter) / nanosec_to_sec(effective_duration_ns);

			// Write plain to the CSV
			/*
			fprintf(
					file_incoming_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
					info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
					info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, received_counter,
					received_counter * complete_packet_size, received_counter * max_payload_size_byte, info.GetMetadata().c_str()
			);
			*/
			// Write nicely formatted to the text
//			char str_target_rate[100];
//			sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
			char str_start_time[100];
			sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
			char str_duration_ms[100];
			sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
//			char str_eff_rate_incl_headers[100];
//			sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
			char str_eff_rate_payload_only[100];
			sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);


			char str_send_size_counter[100];
			sprintf(str_send_size_counter, "%.2f Mbit", byte_to_megabit(send_size_counter));
//			char str_received_incl_headers[100];
//			sprintf(str_received_incl_headers, "%.2f Mbit", byte_to_megabit(received_counter * complete_packet_size));
//			char str_received_payload_only[100];
//			sprintf(str_received_payload_only, "%.2f Mbit", byte_to_megabit(received_counter * max_payload_size_byte));
			fprintf(
					file_outgoing_txt,
					"%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-16s%-16s%-28s%-19" PRIu64 "%-28s" "%s\n",
					info.GetBurstId(),
					info.GetFromNodeId(),
					info.GetToNodeId(),
					str_start_time,
					str_duration_ms,
					str_eff_rate_payload_only,
					send_counter,
					str_send_size_counter,
					info.GetMetadata().c_str()
			);

            nlohmann::ordered_json jsonObject;
            jsonObject["Applicartion ID"] = info.GetBurstId();
            jsonObject["From"] = info.GetFromNodeId();
            jsonObject["To"] = info.GetToNodeId();
            jsonObject["Start time"] = str_start_time;
            jsonObject["Duration"] = str_duration_ms;
            jsonObject["Outgoing rate (payload)"] = str_eff_rate_payload_only;
            jsonObject["Packets sent"] = send_counter;
            jsonObject["Data sent (payload)"] = str_send_size_counter;
            jsonObject["Metadata"] = info.GetMetadata().c_str();
            jsonRtpOutgoing.push_back(jsonObject);
		}

		std::ofstream rtpOutgoingRecord(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_outgoing.json", std::ofstream::out);
		if (rtpOutgoingRecord.is_open()) {
			rtpOutgoingRecord << jsonRtpOutgoing.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			rtpOutgoingRecord.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

		// Incoming bursts
		std::cout << "  > Writing incoming log files" << std::endl;
        nlohmann::ordered_json jsonRtpIncoming;
		for (std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>> p : m_responsible_for_incoming_bursts_rtp) {
			SAGBurstInfoRtp info = p.first;
			Ptr<SAGApplicationLayerRTPReceiver> sagApplicationRtpIncoming = p.second;

			// Fetch data from the application
			uint64_t received_counter = sagApplicationRtpIncoming->GetReceivedCounterOf(info.GetBurstId());
			uint64_t received_size_counter = sagApplicationRtpIncoming->GetReceivedSizeCounterOf(info.GetBurstId());

			// Calculate incoming rate
			int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
			//double rate_incl_headers_megabit_per_s = byte_to_megabit(received_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
			double rate_payload_only_megabit_per_s = byte_to_megabit(received_size_counter) / nanosec_to_sec(effective_duration_ns);

			// Write plain to the CSV
			/*
			fprintf(
					file_incoming_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
					info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
					info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, received_counter,
					received_counter * complete_packet_size, received_counter * max_payload_size_byte, info.GetMetadata().c_str()
			);
			*/
			// Write nicely formatted to the text
//			char str_target_rate[100];
//			sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
			char str_start_time[100];
			sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
			char str_duration_ms[100];
			sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
//			char str_eff_rate_incl_headers[100];
//			sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
			char str_eff_rate_payload_only[100];
			sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);


			char str_received_size_counter[100];
			sprintf(str_received_size_counter, "%.2f Mbit", byte_to_megabit(received_size_counter));
//			char str_received_incl_headers[100];
//			sprintf(str_received_incl_headers, "%.2f Mbit", byte_to_megabit(received_counter * complete_packet_size));
//			char str_received_payload_only[100];
//			sprintf(str_received_payload_only, "%.2f Mbit", byte_to_megabit(received_counter * max_payload_size_byte));
			fprintf(
					file_incoming_txt,
					"%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-16s%-16s%-28s%-19" PRIu64 "%-28s" "%s\n",
					info.GetBurstId(),
					info.GetFromNodeId(),
					info.GetToNodeId(),
					str_start_time,
					str_duration_ms,
					str_eff_rate_payload_only,
					received_counter,
					str_received_size_counter,
					info.GetMetadata().c_str()
			);

            nlohmann::ordered_json jsonObject;
            jsonObject["Applicartion ID"] = info.GetBurstId();
            jsonObject["From"] = info.GetFromNodeId();
            jsonObject["To"] = info.GetToNodeId();
            jsonObject["Start time"] = str_start_time;
            jsonObject["Duration"] = str_duration_ms;
            jsonObject["Incoming rate (payload)"] = str_eff_rate_payload_only;
            jsonObject["Packets received"] = received_counter;
            jsonObject["Data received (payload)"] = str_received_size_counter;
            jsonObject["Metadata"] = info.GetMetadata().c_str();
            jsonRtpIncoming.push_back(jsonObject);
		}

		std::ofstream rtpIncomingRecord(m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_rtp_flows_incoming.json", std::ofstream::out);
		if (rtpIncomingRecord.is_open()) {
			rtpIncomingRecord << jsonRtpIncoming.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			rtpIncomingRecord.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

		// Close files
		std::cout << "  > Closing RTP flow log files:" << std::endl;
//		fclose(file_outgoing_csv);
//		std::cout << "    >> Closed: " << m_sag_bursts_outgoing_csv_filename_rtp << std::endl;
		fclose(file_outgoing_txt);
		std::cout << "    >> Closed: " << m_sag_bursts_outgoing_txt_filename_rtp << std::endl;
//		fclose(file_incoming_csv);
//		std::cout << "    >> Closed: " << m_sag_bursts_incoming_csv_filename_rtp << std::endl;
		fclose(file_incoming_txt);
		std::cout << "    >> Closed: " << m_sag_bursts_incoming_txt_filename_rtp << std::endl;
		// Register completion
		std::cout << "  > Application log files have been written" << std::endl;
		m_basicSimulation->RegisterTimestamp("Write RTP flow log files");
	}
	std::cout << std::endl;

//    std::cout << "STORE SAG APPLICATION RESULTS" << std::endl;
//    // Check if it is enabled explicitly
//    if (!m_enabled) {
//        std::cout << "  > Not enabled, so no SAG application results are written" << std::endl;
//
//    } else {
//
//        // Open files
//        std::cout << "  > Opening SAG application log files:" << std::endl;
//        //FILE* file_outgoing_csv = fopen(m_sag_bursts_outgoing_csv_filename_rtp.c_str(), "w+");
//        //std::cout << "    >> Opened: " << m_sag_bursts_outgoing_csv_filename_rtp << std::endl;
//        //FILE* file_outgoing_txt = fopen(m_sag_bursts_outgoing_txt_filename_rtp.c_str(), "w+");
//        //std::cout << "    >> Opened: " << m_sag_bursts_outgoing_txt_filename_rtp << std::endl;
//        FILE* file_incoming_csv = fopen(m_sag_bursts_incoming_csv_filename_rtp.c_str(), "w+");
//        std::cout << "    >> Opened: " << m_sag_bursts_incoming_csv_filename_rtp << std::endl;
//        FILE* file_incoming_txt = fopen(m_sag_bursts_incoming_txt_filename_rtp.c_str(), "w+");
//        std::cout << "    >> Opened: " << m_sag_bursts_incoming_txt_filename_rtp << std::endl;
//
//        // Header
//        std::cout << "  > Writing sag_application_{incoming, outgoing}.txt headers" << std::endl;
//        /*
//        fprintf(
//                file_outgoing_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-16s%-28s%-28s%s\n",
//                "SAG Applicartion ID", "From", "To", "Target rate", "Start time", "Duration",
//                "Outgoing rate (w/ headers)", "Outgoing rate (payload)", "Packets sent",
//                "Data sent (w/headers)", "Data sent (payload)", "Metadata"
//        );
//        */
//        /*
//        fprintf(
//                file_incoming_txt, "%-16s%-10s%-10s%-20s%-16s%-16s%-28s%-28s%-19s%-28s%-28s%s\n",
//                "SAG Applicartion ID", "From", "To", "Target rate", "Start time", "Duration",
//                "Incoming rate (w/ headers)", "Incoming rate (payload)", "Packets received",
//                "Data received (w/headers)", "Data received (payload)", "Metadata"
//		);
//		*/
//        // Sort ascending to preserve SAG applicartion schedule order
//        struct ascending_paired_sag_application_id_key_send
//        {
//            inline bool operator() (const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>>& a, const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>>& b)
//            {
//                return (a.first.GetBurstId() < b.first.GetBurstId());
//            }
//        };
//        struct ascending_paired_sag_application_id_key_receive
//        {
//            inline bool operator() (const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>>& a, const std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>>& b)
//            {
//                return (a.first.GetBurstId() < b.first.GetBurstId());
//            }
//        };
//        //std::sort(m_responsible_for_outgoing_bursts_rtp.begin(), m_responsible_for_outgoing_bursts_rtp.end(), ascending_paired_sag_application_id_key_send());
//        std::sort(m_responsible_for_incoming_bursts_rtp.begin(), m_responsible_for_incoming_bursts_rtp.end(), ascending_paired_sag_application_id_key_receive());
//
//        // Outgoing bursts
//        /*
//        std::cout << "  > Writing outgoing log files" << std::endl;
//        for (std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPSender>> p : m_responsible_for_outgoing_bursts_rtp) {
//            SAGBurstInfoRtp info = p.first;
//            Ptr<SAGApplicationLayerRTPSender> sagApplicationRtpOutgoing = p.second;
//
//            // Fetch data from the application
//            uint32_t complete_packet_size = 1500;
//            uint32_t max_payload_size_byte = sagApplicationRtpOutgoing->GetMaxPayloadSizeByte();
//            uint64_t sent_counter = sagApplicationRtpOutgoing->GetSentCounterOf(info.GetBurstId());
//
//            // Calculate outgoing rate
//            int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
//            double rate_incl_headers_megabit_per_s = byte_to_megabit(sent_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
//            double rate_payload_only_megabit_per_s = byte_to_megabit(sent_counter * max_payload_size_byte) / nanosec_to_sec(effective_duration_ns);
//
//            // Write plain to the CSV
//            fprintf(
//                    file_outgoing_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
//                    info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
//                    info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, sent_counter,
//                    sent_counter * complete_packet_size, sent_counter * max_payload_size_byte, info.GetMetadata().c_str()
//            );
//
//            // Write nicely formatted to the text
//            char str_target_rate[100];
//            sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
//            char str_start_time[100];
//            sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
//            char str_duration_ms[100];
//            sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
//            char str_eff_rate_incl_headers[100];
//            sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
//            char str_eff_rate_payload_only[100];
//            sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
//            char str_sent_incl_headers[100];
//            sprintf(str_sent_incl_headers, "%.2f Mbit", byte_to_megabit(sent_counter * complete_packet_size));
//            char str_sent_payload_only[100];
//            sprintf(str_sent_payload_only, "%.2f Mbit", byte_to_megabit(sent_counter * max_payload_size_byte));
//            fprintf(
//                    file_outgoing_txt,
//                    "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-16" PRIu64 "%-28s%-28s%s\n",
//                    info.GetBurstId(),
//                    info.GetFromNodeId(),
//                    info.GetToNodeId(),
//                    str_target_rate,
//                    str_start_time,
//                    str_duration_ms,
//                    str_eff_rate_incl_headers,
//                    str_eff_rate_payload_only,
//                    sent_counter,
//                    str_sent_incl_headers,
//                    str_sent_payload_only,
//                    info.GetMetadata().c_str()
//            );
//        }
//        */
//        // Incoming bursts
//        std::cout << "  > Writing incoming log files" << std::endl;
//        for (std::pair<SAGBurstInfoRtp, Ptr<SAGApplicationLayerRTPReceiver>> p : m_responsible_for_incoming_bursts_rtp) {
//            SAGBurstInfoRtp info = p.first;
//            Ptr<SAGApplicationLayerRTPReceiver> sagApplicationRtpIncoming = p.second;
//
//            // Fetch data from the application
//            uint32_t complete_packet_size = 1500;
//            uint32_t max_payload_size_byte = sagApplicationRtpIncoming->GetMaxPayloadSizeByte();
//            uint64_t received_counter = sagApplicationRtpIncoming->GetReceivedCounterOf(info.GetBurstId());
//
//            // Calculate incoming rate
//            int64_t effective_duration_ns = info.GetStartTimeNs() + info.GetDurationNs() >= m_simulation_end_time_ns ? m_simulation_end_time_ns - info.GetStartTimeNs() : info.GetDurationNs();
//            double rate_incl_headers_megabit_per_s = byte_to_megabit(received_counter * complete_packet_size) / nanosec_to_sec(effective_duration_ns);
//            double rate_payload_only_megabit_per_s = byte_to_megabit(received_counter * max_payload_size_byte) / nanosec_to_sec(effective_duration_ns);
//
//            // Write plain to the CSV
//            /*
//            fprintf(
//                    file_incoming_csv, "%" PRId64 ",%" PRId64 ",%" PRId64 ",%f,%" PRId64 ",%" PRId64 ",%f,%f,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s\n",
//                    info.GetBurstId(), info.GetFromNodeId(), info.GetToNodeId(), info.GetTargetRateMegabitPerSec(), info.GetStartTimeNs(),
//                    info.GetDurationNs(), rate_incl_headers_megabit_per_s, rate_payload_only_megabit_per_s, received_counter,
//                    received_counter * complete_packet_size, received_counter * max_payload_size_byte, info.GetMetadata().c_str()
//            );
//            */
//            // Write nicely formatted to the text
//            char str_target_rate[100];
//            sprintf(str_target_rate, "%.2f Mbit/s", info.GetTargetRateMegabitPerSec());
//            char str_start_time[100];
//            sprintf(str_start_time, "%.2f ms", nanosec_to_millisec(info.GetStartTimeNs()));
//            char str_duration_ms[100];
//            sprintf(str_duration_ms, "%.2f ms", nanosec_to_millisec(info.GetDurationNs()));
//            char str_eff_rate_incl_headers[100];
//            sprintf(str_eff_rate_incl_headers, "%.2f Mbit/s", rate_incl_headers_megabit_per_s);
//            char str_eff_rate_payload_only[100];
//            sprintf(str_eff_rate_payload_only, "%.2f Mbit/s", rate_payload_only_megabit_per_s);
//            char str_received_incl_headers[100];
//            sprintf(str_received_incl_headers, "%.2f Mbit", byte_to_megabit(received_counter * complete_packet_size));
//            char str_received_payload_only[100];
//            sprintf(str_received_payload_only, "%.2f Mbit", byte_to_megabit(received_counter * max_payload_size_byte));
//            fprintf(
//                    file_incoming_txt,
//                    "%-16" PRId64 "%-10" PRId64 "%-10" PRId64 "%-20s%-16s%-16s%-28s%-28s%-19" PRIu64 "%-28s%-28s%s\n",
//                    info.GetBurstId(),
//                    info.GetFromNodeId(),
//                    info.GetToNodeId(),
//                    str_target_rate,
//                    str_start_time,
//                    str_duration_ms,
//                    str_eff_rate_incl_headers,
//                    str_eff_rate_payload_only,
//                    received_counter,
//                    str_received_incl_headers,
//                    str_received_payload_only,
//                    info.GetMetadata().c_str()
//            );
//        }
//
//        // Close files
//        std::cout << "  > Closing SAG application log files:" << std::endl;
//        //fclose(file_outgoing_csv);
//        //std::cout << "    >> Closed: " << m_sag_bursts_outgoing_csv_filename_rtp << std::endl;
//        //fclose(file_outgoing_txt);
//        //std::cout << "    >> Closed: " << m_sag_bursts_outgoing_txt_filename_rtp << std::endl;
//        fclose(file_incoming_csv);
//        std::cout << "    >> Closed: " << m_sag_bursts_incoming_csv_filename_rtp << std::endl;
//        fclose(file_incoming_txt);
//        std::cout << "    >> Closed: " << m_sag_bursts_incoming_txt_filename_rtp << std::endl;
//        // Register completion
//        std::cout << "  > SAG application log files have been written" << std::endl;
//        m_basicSimulation->RegisterTimestamp("Write SAG application log files");
//    }
//    std::cout << std::endl;
}





/**
 * Read in the SAG application Rtp schedule.
 *
 * @param filename                  File name of the sag_burst_schedule.csv
 * @param topology                  Topology
 * @param simulation_end_time_ns    Simulation end time (ns) : all SAG applicaitons must start less than this value
*/
std::vector<SAGBurstInfoRtp> read_sag_applicaton_schedule_rtp(const std::string& filename, Ptr<TopologySatelliteNetwork> topology, const int64_t simulation_end_time_ns)
{
//    // Schedule to put in the data
//	std::vector<SAGBurstInfoRtp> schedule;
//
//	// Check that the file exists
//	if (!file_exists(filename)) {
//		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
//	}
//
//	uint32_t gsIdStart = topology->GetNumSatellites();
//	//uint32_t gsIdStart = 0;
//
//	// Open file
//	std::string line;
//	std::ifstream schedule_file(filename);
//	if (schedule_file) {
//
//		// Go over each line
//		size_t line_counter = 0;
//		int64_t prev_start_time_ns = 0;
//		while (getline(schedule_file, line)) {
//
//			// Split on ,
//			std::vector<std::string> comma_split = split_string(line, ",", 8);
//
//			// Fill entry
//			int64_t rtp_flow_id = parse_positive_int64(comma_split[0]);
//			if (rtp_flow_id != (int64_t) line_counter) {
//				throw std::invalid_argument(format_string("RTP flow ID is not ascending by one each line (violation: %" PRId64 ")\n", rtp_flow_id));
//			}
//			int64_t from_node_id = gsIdStart + parse_positive_int64(comma_split[1]);
//			int64_t to_node_id = gsIdStart + parse_positive_int64(comma_split[2]);
//			double size_byte = parse_positive_double(comma_split[3]);
//			int64_t start_time_ns = parse_positive_int64(comma_split[4]);
//			int64_t duration_ns = parse_positive_int64(comma_split[5]);
//			std::string additional_parameters = comma_split[6];
//			std::string metadata = comma_split[7];
//			metadata.erase(std::remove_if(metadata.begin(),metadata.end(),::isspace),metadata.end());
//
//			// Must be weakly ascending start time
//			if (prev_start_time_ns > start_time_ns) {
//				throw std::invalid_argument(format_string("Start time is not weakly ascending (on line with RTP flow ID: %" PRId64 ", violation: %" PRId64 ")\n", rtp_flow_id, start_time_ns));
//			}
//			prev_start_time_ns = start_time_ns;
//
//			// Check node IDs
//			if (from_node_id == to_node_id) {
//				throw std::invalid_argument(format_string("RTP flow to itself at node ID: %" PRId64 ".", to_node_id));
//			}
//
//			// Check endpoint validity
//			if (!topology->IsValidEndpoint(from_node_id)) {
//				throw std::invalid_argument(format_string("Invalid from-endpoint for a schedule entry based on topology: %d", from_node_id));
//			}
//			if (!topology->IsValidEndpoint(to_node_id)) {
//				throw std::invalid_argument(format_string("Invalid to-endpoint for a schedule entry based on topology: %d", to_node_id));
//			}
//
//			// Check start time
//			if (start_time_ns >= simulation_end_time_ns) {
//				throw std::invalid_argument(format_string(
//						"RTP flow %" PRId64 " has invalid start time %" PRId64 " >= %" PRId64 ".",
//						rtp_flow_id, start_time_ns, simulation_end_time_ns
//				));
//			}
//
//			// Put into schedule
//			schedule.push_back(SAGBurstInfoRtp(rtp_flow_id, from_node_id, to_node_id, size_byte, start_time_ns, duration_ns, additional_parameters, metadata));
//
//			// Next line
//			line_counter++;
//
//		}
//
//		// Close file
//		schedule_file.close();
//
//	} else {
//		throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
//	}
//
//	return schedule;




    // Schedule to put in the data
    std::vector<SAGBurstInfoRtp> schedule;

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
		jsonns::application_schedules rtps = j.at("rtp_flows");
		std::vector<jsonns::application_schedule> my_application_schedules_cur = rtps.my_application_schedules;


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
			schedule.push_back(SAGBurstInfoRtp(sag_application_id, from_node_id, to_node_id, target_rate_megabit_per_s, start_time_ns, duration_ns, additional_parameters, metadata));

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
