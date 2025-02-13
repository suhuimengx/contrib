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

#include "sag_application_schedule_3gpphttp.h"
#include "ns3/cppjson2structure.hh"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("ThreeGppHttpApplicationScheduler");

SAGApplicationSchedulerThreeGppHttp::SAGApplicationSchedulerThreeGppHttp(Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology)
:SAGApplicationScheduler(basicSimulation, topology)
{
    printf("APPLICATION SCHEDULER 3GppHttp\n");

    // Check if it is enabled explicitly
    //m_enabled = parse_boolean(m_basicSimulation->GetConfigParamOrDefault("enable_sag_application_scheduler_3gpp_http", "false"));
    m_enabled = ReadApplicationEnable("3gpp_http_flow");
    if (!m_enabled) {
        std::cout << "  > Not enabled explicitly, so disabled" << std::endl;

    } else {
        std::cout << "  > 3GppHttp flow scheduler is enabled" << std::endl;

        // Read logging
        ReadLoggingForApplicationIds("http");

        // Read schedule
        std::vector<SAGBurstInfo3GppHttp> complete_schedule = read_3GppHttp_flow_schedule(
                m_basicSimulation->GetRunDir() + "/config_traffic/application_schedule_3gpphttp.json",
                m_topology,
                m_simulation_end_time_ns
        );

        // Check that the 3GppHttp flow IDs exist in the logging
        for (int64_t threeGppHttp_flow_id : m_enable_logging_for_sag_application_ids) {
            if ((size_t) threeGppHttp_flow_id >= complete_schedule.size()) {
                throw std::invalid_argument("Invalid 3GppHttp flow ID in sag_applicaiton_enable_logging_for_sag_application_threeGppHttp_ids: " + std::to_string(threeGppHttp_flow_id));
            }
			if(m_system_id == 0){
	    		mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/http_" + std::to_string(threeGppHttp_flow_id));
			}
        }

        // Filter the schedule to only have applications starting at nodes which are part of this system
        if (m_enable_distributed) {
            std::vector<SAGBurstInfo3GppHttp> filtered_schedule;
            for (SAGBurstInfo3GppHttp &entry : complete_schedule) {
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


        m_flows_csv_filename =
                m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_3GppHttp_flows.csv";
        m_flows_txt_filename =
                m_basicSimulation->GetLogsDir() + "/system_" + std::to_string(m_system_id) + "_3GppHttp_flows.txt";

        // Remove files if they are there
        remove_file_if_exists(m_flows_csv_filename);
        remove_file_if_exists(m_flows_txt_filename);
        printf("  > Removed previous flow log files if present\n");
        m_basicSimulation->RegisterTimestamp("Remove previous flow log files");



//        // Install sink on endpoint node
//        uint16_t my_port_num = 3026;
//        uint16_t dst_port_num = 3026;

        std::cout << "  > Setting up applications on endpoint nodes" << std::endl;
        for (SAGBurstInfo3GppHttp entry : complete_schedule) {
        	int64_t src = entry.GetFromNodeId();
        	int64_t dst = entry.GetToNodeId();
        	Ipv4Address serverAddress = m_nodes.Get(dst)->GetObject<Ipv4L3Protocol>()->GetAddress(1,0).GetLocal();


        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[src] == m_system_id){
        		// Create HTTP client helper
        		ThreeGppHttpClientHelper clientHelper (serverAddress);

        		// Install HTTP client
        		ApplicationContainer clientApps = clientHelper.Install (m_nodes.Get(src));
        		Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

        		m_responsible_for_outgoing_bursts.push_back(std::make_pair(entry, httpClient));

        		if(m_enable_logging_for_sag_application_ids.find(entry.GetBurstId()) != m_enable_logging_for_sag_application_ids.end()){
            		// Example of connecting to the trace sources
            		httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ThreeGppHttpClient::ClientMainObjectReceived, httpClient));
            		httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ThreeGppHttpClient::ClientEmbeddedObjectReceived, httpClient));
            		httpClient->TraceConnectWithoutContext ("RxRtt", MakeCallback (&ThreeGppHttpClient::CompleteObjectRxRTT, httpClient));
            		httpClient->TraceConnectWithoutContext ("RxDelay", MakeCallback (&ThreeGppHttpClient::CompleteObjectRxDelay, httpClient));
            		//httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));
        		}

        		// Stop browsing after 30 minutes
        		clientApps.Start(NanoSeconds(entry.GetStartTimeNs()));
        		clientApps.Stop (NanoSeconds (entry.GetDurationNs()) + NanoSeconds(entry.GetStartTimeNs()));







				// Setup the virtual application, just for minimum hop routing
				SAGApplicationHelper sagApplicationHelper(m_basicSimulation);
				ApplicationContainer app = sagApplicationHelper.Install(m_nodes.Get(src));
				Ptr<SAGApplicationLayer> sagApplicationLayer = app.Get(0)->GetObject<SAGApplicationLayer>();

				// must be both set in sinks and senders
				sagApplicationLayer->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
				sagApplicationLayer->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));



        	}

        	if (!m_enable_distributed || m_distributed_node_system_id_assignment[dst] == m_system_id){

        		// Create HTTP server helper
				ThreeGppHttpServerHelper serverHelper (serverAddress);

				// Install HTTP server
				ApplicationContainer serverApps = serverHelper.Install (m_nodes.Get(dst));
				Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

				m_responsible_for_incoming_bursts.push_back(std::make_pair(entry, httpServer));

				// Example of connecting to the trace sources
				//httpServer->TraceConnectWithoutContext ("ConnectionEstablished",MakeCallback (&SAGApplicationSchedulerThreeGppHttp::ServerConnectionEstablished, this));
				//httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&SAGApplicationSchedulerThreeGppHttp::MainObjectGenerated, this));
				//httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&SAGApplicationSchedulerThreeGppHttp::EmbeddedObjectGenerated, this));
				//httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

				// Setup HTTP variables for the server
				PointerValue varPtr;
				httpServer->GetAttribute ("Variables", varPtr);
				Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
				httpVariables->SetMainObjectSizeMean (102400); // 100kB
				httpVariables->SetMainObjectSizeStdDev (40960); // 40kB




				// Setup the virtual application, just for minimum hop routing
				SAGApplicationHelper sagApplicationHelper(m_basicSimulation);
				ApplicationContainer app = sagApplicationHelper.Install(m_nodes.Get(dst));
				Ptr<SAGApplicationLayer> sagApplicationLayer = app.Get(0)->GetObject<SAGApplicationLayer>();

				// must be both set in sinks and senders
				sagApplicationLayer->SetSourceNode(m_nodes.Get(entry.GetFromNodeId()));
				sagApplicationLayer->SetDestinationNode(m_nodes.Get(entry.GetToNodeId()));




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

//        	my_port_num++;
//        	dst_port_num++;


        }
        std::cout << "  > Setting up traffic 3GppHttp flow starter" << std::endl;
        m_basicSimulation->RegisterTimestamp("Setup traffic 3GppHttp flow starter");






    }

    std::cout << std::endl;
}










void SAGApplicationSchedulerThreeGppHttp::WriteResults() {

	for(auto flow: m_responsible_for_outgoing_bursts){
		SAGBurstInfo3GppHttp entry = flow.first;
		Ptr<ThreeGppHttpClient> client = flow.second;
		nlohmann::ordered_json jsonContents;
	    NS_ASSERT_MSG(client->m_name.size()==client->m_rtt.size(), "Wrong Record!");
		for(uint32_t i = 0; i < client->m_name.size(); i++){
			nlohmann::ordered_json jsonContent;
			jsonContent["object_name"] = client->m_name[i];
			jsonContent["time_stamp_s"] = client->m_time[i];
			jsonContent["object_size_bytes"] = client->m_size[i];
			jsonContent["rtt_s"] = client->m_rtt[i];
			jsonContent["delay_s"] = client->m_delay[i];
			jsonContents.push_back(jsonContent);

		}
		std::ofstream httpRes(m_basicSimulation->GetRunDir() + "/results/network_results/object_statistics/http_" + std::to_string(entry.GetBurstId())+"/http_" + std::to_string(entry.GetBurstId())+"_log.json", std::ofstream::out);
		if (httpRes.is_open()) {
			httpRes << jsonContents.dump(4);  // 使用缩进格式将 JSON 内容写入文件
			httpRes.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}
	}

}





/**
 * Read in the SAG application Tcp schedule.
 *
 * @param filename                  File name of the schedule.csv
 * @param topology                  Topology
 * @param simulation_end_time_ns    Simulation end time (ns) : all flows must start less than this value
*/
std::vector<SAGBurstInfo3GppHttp> read_3GppHttp_flow_schedule(const std::string& filename, Ptr<TopologySatelliteNetwork> topology, const int64_t simulation_end_time_ns) {

//    // Schedule to put in the data
//    std::vector<SAGBurstInfo3GppHttp> schedule;
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
//            schedule.push_back(SAGBurstInfo3GppHttp(tcp_flow_id, from_node_id, to_node_id, size_byte, start_time_ns, duration_ns, additional_parameters, metadata));
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
    std::vector<SAGBurstInfo3GppHttp> schedule;

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
		jsonns::application_schedules_3gpphttp threegpphttps = j.at("3gpphttp_flows");
		std::vector<jsonns::application_schedule_3gpphttp> my_application_schedules_cur = threegpphttps.my_application_schedules;


		// Go over each line
		size_t line_counter = 0;
		int64_t prev_start_time_ns = 0;
		for(uint32_t i = 0; i < my_application_schedules_cur.size(); i++) {

			jsonns::application_schedule_3gpphttp cur_application_schedule = my_application_schedules_cur[i];

			// Fill entry
			int64_t sag_application_id = parse_positive_int64(cur_application_schedule.flow_id);
			if (sag_application_id != (int64_t) line_counter) {
				throw std::invalid_argument(format_string("Application ID is not ascending by one each line (violation: %" PRId64 ")\n", sag_application_id));
			}
			int64_t from_node_id = gsIdStart + parse_positive_int64(cur_application_schedule.sender);
			int64_t to_node_id = gsIdStart + parse_positive_int64(cur_application_schedule.receiver);
			int64_t start_time_ns = parse_positive_int64(cur_application_schedule.start_time_ns);
			int64_t duration_ns = parse_positive_int64(cur_application_schedule.duration_time_ns);


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
			schedule.push_back(SAGBurstInfo3GppHttp(sag_application_id, from_node_id, to_node_id, start_time_ns, duration_ns));

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


//void
//SAGApplicationSchedulerThreeGppHttp::ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
//{
//  NS_LOG_INFO ("Client has established a connection to the server.");
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::MainObjectGenerated (uint32_t size)
//{
//  NS_LOG_INFO ("Server generated a main object of " << size << " bytes.");
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::EmbeddedObjectGenerated (uint32_t size)
//{
//  NS_LOG_INFO ("Server generated an embedded object of " << size << " bytes.");
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::ServerTx (Ptr<const Packet> packet)
//{
//  NS_LOG_INFO ("Server sent a packet of " << packet->GetSize () << " bytes.");
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::ClientRx (Ptr<const Packet> packet, const Address &address)
//{
//  NS_LOG_INFO ("Client received a packet of " << packet->GetSize () << " bytes from " << address);
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
//{
//  Ptr<Packet> p = packet->Copy ();
//  ThreeGppHttpHeader header;
//  p->RemoveHeader (header);
//  if (header.GetContentLength () == p->GetSize ()
//	  && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
//	{
//	  NS_LOG_INFO ("Client has successfully received a main object of "
//				   << p->GetSize () << " bytes.");
//	  m_name.push_back("A main object");
//	  m_size.push_back(p->GetSize ());
//	  m_time.push_back(Simulator::Now().GetSeconds());
//	}
//  else
//	{
//	  NS_LOG_INFO ("Client failed to parse a main object. ");
//	}
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
//{
//  Ptr<Packet> p = packet->Copy ();
//  ThreeGppHttpHeader header;
//  p->RemoveHeader (header);
//  if (header.GetContentLength () == p->GetSize ()
//	  && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
//	{
//	  NS_LOG_INFO ("Client has successfully received an embedded object of "
//				   << p->GetSize () << " bytes.");
//	  m_name.push_back("An embedded object");
//	  m_size.push_back(p->GetSize ());
//	  m_time.push_back(Simulator::Now().GetSeconds());
//	}
//  else
//	{
//	  NS_LOG_INFO ("Client failed to parse an embedded object. ");
//	}
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::CompleteObjectRxRTT(const Time & t, const Address &address){
//
//	  NS_LOG_INFO ("Client received a complete object with rtt "<<t.GetSeconds()<<" s"<<" from " << address);
//	  m_rtt.push_back(t.GetSeconds());
//
//}
//
//void
//SAGApplicationSchedulerThreeGppHttp::CompleteObjectRxDelay(const Time & t, const Address &address){
//	  NS_LOG_INFO ("Client received a complete object with delay "<<t.GetSeconds()<<" s"<<" from " << address);
//	  m_delay.push_back(t.GetSeconds());
//}

}
