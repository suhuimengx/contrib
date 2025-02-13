/*
 * Author: Kolimn-Zhang and Yuru Liu
 */

#include "trajectory.h"
#include "ns3/sgp4coord.h"
#include "ns3/cppmap3d.hh"
#include "ns3/cppjson2structure.hh"



namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("Trajectory");

    Trajectory::Trajectory (Ptr<BasicSimulation> basicSimulation, Ptr<TopologySatelliteNetwork> topology){
    	//if(basicSimulation->GetSystemId() != 0) return;
    	m_basicSimulation = basicSimulation;
    	m_topology = topology;
		ReadTimeAxleJson(basicSimulation);
    }

    Trajectory::~Trajectory (){

	}

	void
	Trajectory::SetBasicSimHandle(Ptr<BasicSimulation> basicSimulation){
		m_basicSimulation = basicSimulation;
	}

	void
	Trajectory::ReadTimeAxleJson(Ptr<BasicSimulation> basicSimulation){
		m_time_IntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"))/1e9;
		m_time_end = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("simulation_end_time_ns"))/1e9;
	}


	void
	Trajectory::WriteTimeAxleJson(){

		std::vector<nlohmann::ordered_json> json_v;
		NodeContainer allNodes = m_topology->GetNodes();
		Ptr<Node> randomSatelliteNode = allNodes.Get(0);
		JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
		JulianDate end = start + Seconds(m_time_end);

		std::string start_string = start.ToStringiso();
		std::string end_string = end.ToStringiso();
		std::string time = start_string + '/' + end_string;
		nlohmann::ordered_json jsonContent = {
			{"id", "document"},
			{"name", "simple"},
			{"version", "1.0"},
			{"clock", {
				{"interval", time},
				{"currentTime", start_string},
				{"multiplier", 20},
				{"range", "LOOP_STOP"},
				{"step", "SYSTEM_CLOCK_MULTIPLIER"},
			}}
		};
		json_v.push_back(jsonContent);

		std::ofstream TmieAxleResult(m_basicSimulation->GetRunDir() + "/results/cesium_results/TimeAxle.json", std::ofstream::out);
		if (TmieAxleResult.is_open()) {
			for (const auto& jsonObject : json_v) {
				TmieAxleResult << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				//TmieAxleResult << std::endl;  // 每个 JSON 对象之间添加换行
			}
			TmieAxleResult.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

	}



	void
	Trajectory::ReadSatPosition(int j){

		std::string filename = m_basicSimulation->GetRunDir() + "/config_topology"+"/system_"+ to_string(m_basicSimulation->GetSystemId()) + "_coordinates/satellite_"+std::to_string(j)+".txt";

		// Check that the file exists
		if (!file_exists(filename)) {
			throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
		}
		else{
			ifstream satfile(filename);
			if (satfile) {
        		std::string line;   
				while (std::getline(satfile, line)) {
					std::istringstream iss(line);
    				std::string s;
					sat_position satp;
					satp.id=0;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.time;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.positionx;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.positiony;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.positionz;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.latitude;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.longitude;
					std::getline(iss, s, ',');
    				std::stringstream(s) >> satp.altitude;
					m_satp.push_back(satp);				
        		}
				satfile.close();
			}
			else {
				std::cout << "Failed to open file: " << filename << std::endl;
			}	
		}
	}


	void
	Trajectory::WriteSatPositionJson(){
		remove_dir_and_subfile_if_exists(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_position_system_"+to_string(m_basicSimulation->GetSystemId()));
		mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_position_system_"+to_string(m_basicSimulation->GetSystemId()));
		remove_dir_and_subfile_if_exists(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_coverage_system_"+to_string(m_basicSimulation->GetSystemId()));
		mkdir_force_if_not_exists(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_coverage_system_"+to_string(m_basicSimulation->GetSystemId()));
		//NodeContainer allNodes = m_topology->GetSatelliteNodes();
		//std::vector<Ptr<Constellation>> allCons = m_topology->GetConstellations();
		NodeContainer nodesCurSystem = m_topology->GetCurrentSystemNodes();

		int i;
	    const double pi = 3.14159265358979311599796346854;

		for(uint32_t sn = 0; sn < nodesCurSystem.GetN(); sn++){
			Ptr<Node> nCS = nodesCurSystem.Get(sn);
			Ptr<Constellation> cons = m_topology->FindConstellationBySatId(nCS->GetId());
			//NodeContainer allNodesinCons = cons->GetNodes();
			std::vector<double> color = cons->GetColorNumbers();
			double arclen = cons->GetArcLength();
			double h = cons->GetAltitude()*1000*2;

			if(1){
				int j = nCS->GetId();
				ReadSatPosition(j);
				std::vector<nlohmann::ordered_json> json_v;
				nlohmann::ordered_json json_v_coverage;

				std::vector<float> info;
				std::vector<float> info_coverage;

				for (i = 0; i < int(m_satp.size()); i++) {
					info.push_back(m_satp[i].time);
					info.push_back(m_satp[i].positionx);
					info.push_back(m_satp[i].positiony);
					info.push_back(m_satp[i].positionz);
					double out_x, out_y, out_z;
					cppmap3d::geodetic2ecef(
						m_satp[i].latitude*pi/180,
						m_satp[i].longitude*pi/180,
					    0,
					    out_x,
					    out_y,
					    out_z,
						cppmap3d::Ellipsoid::WGS72
					);
					info_coverage.push_back(m_satp[i].time);
					info_coverage.push_back(out_x);
					info_coverage.push_back(out_y);
					info_coverage.push_back(out_z);

				}

				Ptr<Node> randomSatelliteNode = nCS;
				JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
				JulianDate end = start + Seconds(m_time_end);

				std::string start_string = start.ToStringiso();
				std::string end_string = end.ToStringiso();
				std::string time = start_string + '/' + end_string;
				auto id = j;


				nlohmann::ordered_json jsonContent = {
					{"id", id},
					{"name", id},
					{"availability", time},
					{"description", ""},
					{"position", {
						{"interpolationAlgorithm", "LAGRANGE"},
						{"interpolationDegree", 5},
						{"referenceFrame", "FIXED"},
						{"epoch", start_string},
						{"cartesian", info}
					}},
					{"point", {
						{"show", true},
						{"color", {
								{"rgba", {color[0], color[1], color[2], color[3]*255}}
						}},
						{"pixelSize", 7},
					}}
				};
				json_v.push_back(jsonContent);
				//
				std::ofstream SATResult(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_position_system_"+to_string(m_basicSimulation->GetSystemId())+"/sat_position_"+std::to_string(j)+".json", std::ofstream::out);
				if (SATResult.is_open()) {
					for (const auto& jsonObject : json_v) {
						SATResult << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
						SATResult << std::endl;  // 每个 JSON 对象之间添加换行
					}
					SATResult.close();
					m_satp.clear();
					//std::cout << "JSON file created successfully." << std::endl;
				} else {
					std::cout << "Failed to create JSON file." << std::endl;
				}




				// coverage
				nlohmann::ordered_json jsonCoverageContent = {
					{"id", id},
					{"name", id},
					{"availability", time},
					{"description", ""},
					{"position", {
						{"interpolationAlgorithm", "LAGRANGE"},
						{"interpolationDegree", 5},
						{"referenceFrame", "FIXED"},
						{"epoch", start_string},
						{"cartesian", info_coverage}
					}},
					{"cylinder", {
						{"length", h},
						{"topRadius", 0},
						{"bottomRadius", arclen},
						{"material", {
							{"solidColor", {
								{"color", {
									{"rgba", {color[0], color[1], color[2], color[3]*255}}
								}}
							}}
				        }}
					}}
				};
				json_v_coverage.push_back(jsonCoverageContent);
				//
				std::ofstream SATResultCoverage(m_basicSimulation->GetRunDir() + "/results/cesium_results/sat_coverage_system_"+to_string(m_basicSimulation->GetSystemId())+"/sat_coverage_"+std::to_string(j)+".json", std::ofstream::out);
				if (SATResultCoverage.is_open()) {
					SATResultCoverage << json_v_coverage.dump(4);  // 使用缩进格式将 JSON 内容写入文件
					SATResultCoverage.close();
					m_satp.clear();
					//std::cout << "JSON file created successfully." << std::endl;
				} else {
					std::cout << "Failed to create JSON file." << std::endl;
				}

			}
		}

	}

	void
	Trajectory::WriteGSPositionJson(){

//		int i;
		nlohmann::ordered_json json_array;
		std::vector<Ptr<GroundStation>> gnds = m_topology->GetGroundStationsReal();
		
		for(auto gnd: gnds)
		{
			auto id = gnd->GetGid();
			auto name = gnd->GetName();
			double positionx = gnd->GetLongitude();
			double positiony = gnd->GetLatitude();
			double positionz = gnd->GetElevation();

//			auto id = m_gsp[i].id;
//			auto name = m_gsp[i].name;
//			float positionx = m_gsp[i].positionx;
//			float positiony = m_gsp[i].positiony;
//			float positionz = m_gsp[i].positionz;

			NodeContainer allNodes = m_topology->GetNodes();
			Ptr<Node> randomSatelliteNode = allNodes.Get(0);
			JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
			JulianDate end = start + Seconds(m_time_end);

			std::string start_string = start.ToStringiso();
			std::string end_string = end.ToStringiso();
			std::string time = start_string + '/' + end_string;
			nlohmann::ordered_json jsonContent = {
				{"id", id},
				{"name", name},
				{"availability", time},
				{"description", ""},			
				{"position", {
					{"cartographicDegrees", {positionx, positiony, positionz}}
				}},
				{"point", {
					{"show", true},
					{"color", {
						{"rgba", {255, 255, 0, 255}}
					}},
					{"pixelSize", 7},
				}}
			};
			//json_v.push_back(jsonContent);
			json_array.push_back(jsonContent);
		}

		std::ofstream GSResult(m_basicSimulation->GetRunDir() + "/results/cesium_results/gs_position.json", std::ofstream::out);
		if (GSResult.is_open()) {
			if(json_array.empty()){
				nlohmann::json data = nlohmann::json::array();
				GSResult << data.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				GSResult.close();
			}
			else{
				GSResult << json_array.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				GSResult.close();
				//std::cout << "JSON file created successfully." << std::endl;
			}
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

	}

	void
	Trajectory::WriteAirCraftsPositionJson(){

		nlohmann::ordered_json json_array;
		std::vector<Ptr<GroundStation>> aircrafts = m_topology->GetAirCraftsReal();
        int64_t dynamicStateUpdateIntervalNs = parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));

		for(uint32_t i = 0; i < aircrafts.size(); i++)
		{
			auto gnd = aircrafts[i];
			std::vector<Vector> positions = m_topology->GetAirCraftsPositions()[i];
			float time_s = 0; // s
			std::vector<float> info;
			for(auto p: positions){
				info.push_back(time_s);
				info.push_back(p.x);
				info.push_back(p.y);
				info.push_back(p.z);
				time_s += dynamicStateUpdateIntervalNs/1e9;
			}
			auto id = gnd->GetGid();
			auto name = gnd->GetName();

			NodeContainer allNodes = m_topology->GetNodes();
			Ptr<Node> randomSatelliteNode = allNodes.Get(0);
			JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
			JulianDate end = start + Seconds(m_time_end);

			std::string start_string = start.ToStringiso();
			std::string end_string = end.ToStringiso();
			std::string time = start_string + '/' + end_string;
			nlohmann::ordered_json jsonContent = {
				{"id", id},
				{"name", name},
				{"availability", time},
				{"description", ""},
				{"position", {
					{"cartographicDegrees", info}
				}},
				{"point", {
					{"show", true},
					{"color", {
						{"rgba", {255, 255, 0, 255}}
					}},
					{"pixelSize", 7},
				}}
			};
			//json_v.push_back(jsonContent);
			json_array.push_back(jsonContent);
		}

		std::ofstream GSResult(m_basicSimulation->GetRunDir() + "/results/cesium_results/air_craft_position.json", std::ofstream::out);
		if (GSResult.is_open()) {
			GSResult << json_array.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//			for (const auto& jsonObject : json_v) {
//				GSResult << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//				GSResult << std::endl;  // 每个 JSON 对象之间添加换行
//			}
			GSResult.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}



	}


	void
	Trajectory::WriteAdjJson(){

		int i;
		auto outageLinks = m_topology->GetInterSatelliteOutageLinkDetails();
		//std::vector<nlohmann::ordered_json> json_v;
		nlohmann::ordered_json json_v;
		NodeContainer allNodes = m_topology->GetNodes();
		std::vector<Ptr<Constellation>> allCons = m_topology->GetConstellations();
		Ptr<Node> randomSatelliteNode = allNodes.Get(0);
		
		for(Ptr<Constellation> cons: allCons){
			std::vector<std::pair<uint32_t, uint32_t>> adj = cons->GetIslFromToUnique();
			std::vector<double> color = cons->GetColorNumbers();
			for(i=0;i<(int)adj.size();i++)
			{
				string id = std::to_string(adj[i].first) + "-to-" + std::to_string(adj[i].second);
				string name = std::to_string(adj[i].first) + " to " + std::to_string(adj[i].second);

				string ref1 = std::to_string(adj[i].first) + "#position";
				string ref2 = std::to_string(adj[i].second) + "#position";

				OutageLink link1 = OutageLink(adj[i].first, adj[i].second);
				auto iter = find(outageLinks.begin(), outageLinks.end(), link1);
				if(iter == outageLinks.end()){
					nlohmann::ordered_json jsonContent = {
						{"id", id},
						{"name", name},
						{"description", ""},
						{"polyline", {
							{"show", true},
							{"width", 1},
							{"material", {
								{"solidColor", {
									{"color", {
											{"rgba", {color[0], color[1], color[2], color[3]*255}}
									}}
								}}
							}},
							{"arcType", "NONE"},
							{"positions", {
								{"references", {ref1, ref2}}
							}}
						}}
					};

					json_v.push_back(jsonContent);
				}
				else{
					std::vector<std::pair<double, double>> timeIntervals = iter->outageTimeInterval;

					JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Seconds(timeIntervals[0].first);
					JulianDate end = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Seconds(timeIntervals[timeIntervals.size()-1].second);
					std::string start_string = start.ToStringiso();
					std::string end_string = end.ToStringiso();

					JulianDate simulation_start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
					JulianDate simulation_end =  randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Simulator::Now();
					std::string simulation_start_string = simulation_start.ToStringiso();
					std::string simulation_end_string = simulation_end.ToStringiso();


					// 创建 JSON 数组 show
					nlohmann::ordered_json showArray;
					nlohmann::ordered_json colorArray;
					if(simulation_start_string != start_string )
					{
						std::string temp_time = simulation_start_string + '/' +start_string;
						nlohmann::ordered_json TempshowArray = {
							{ "interval", temp_time },
							{ "boolean", true }
						};
						showArray.push_back(TempshowArray);

						nlohmann::ordered_json TempcolorArray = {
							{ "interval", temp_time },
							{ "rgba",  {color[0], color[1], color[2], color[3]*255}}
						};
						colorArray.push_back(TempcolorArray);
					}

					for(uint32_t j = 0; j < std::size(timeIntervals); j++)
					{
						auto curstart = simulation_start + Seconds(timeIntervals[j].first);
						auto curend = simulation_start + Seconds(timeIntervals[j].second);
						std::string time = curstart.ToStringiso()+'/' +curend.ToStringiso();

//						nlohmann::ordered_json showArray_duation ={
//							{ "interval", time },
//							{ "boolean", false }
//						};
						nlohmann::ordered_json showArray_duation ={
							{ "interval", time },
							{ "boolean", true }
						};
						showArray.push_back(showArray_duation);

						nlohmann::ordered_json colorArray_duation = {
							{ "interval", time },
							{ "rgba",  {255.0, 0.0, 0.0, 255.0}}
						};
						colorArray.push_back(colorArray_duation);

						if(j != std::size(timeIntervals)-1)
						{
							auto curstart1 = simulation_start + Seconds(timeIntervals[j].second);
							auto curend1 = simulation_start + Seconds(timeIntervals[j+1].first);
							std::string time1 = curstart1.ToStringiso()+'/' +curend1.ToStringiso();
							showArray_duation ={
								{ "interval", time1 },
								{ "boolean", true }
							};
							showArray.push_back(showArray_duation);

							colorArray_duation = {
								{ "interval", time1 },
								{ "rgba",  {color[0], color[1], color[2], color[3]*255}}
							};
							colorArray.push_back(colorArray_duation);
						}
					}


					if(end_string != simulation_end_string)
					{
						std::string temp_time = end_string + '/' +simulation_end_string;
						nlohmann::ordered_json TempshowArray = {
							{ "interval", temp_time },
							{ "boolean", true }
						};
						showArray.push_back(TempshowArray);

						nlohmann::ordered_json TempcolorArray = {
							{ "interval", temp_time },
							{ "rgba",  {color[0], color[1], color[2], color[3]*255}}
						};
						colorArray.push_back(TempcolorArray);
					}


					nlohmann::ordered_json jsonContent = {
						{"id", id},
						{"name", name},
						{"description", ""},
						{"polyline", {
							{"show", showArray},
							{"width", 1},
							{"material", {
								{"solidColor", {
									{"color", colorArray}
								}}
							}},
							{"arcType", "NONE"},
							{"positions", {
								{"references", {ref1, ref2}}
							}}
						}}
					};

					json_v.push_back(jsonContent);

				}


			}


		}

		std::ofstream AdjResult(m_basicSimulation->GetRunDir() + "/results/cesium_results/adj_results.json", std::ofstream::out);
		if (AdjResult.is_open()) {
			AdjResult << json_v.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//			for (const auto& jsonObject : json_v) {
//				AdjResult << jsonObject.dump(4);  // 使用缩进格式将 JSON 内容写入文件
//				AdjResult << std::endl;  // 每个 JSON 对象之间添加换行
//			}
			AdjResult.close();
			//std::cout << "JSON file created successfully." << std::endl;
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

	}


	void
	Trajectory::WriteHandoverJson(){

		nlohmann::ordered_json json_v;
		NodeContainer allNodes = m_topology->GetNodes();
		Ptr<Node> randomSatelliteNode = allNodes.Get(0);
		JulianDate simulation_start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime();
		JulianDate simulation_end =  randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Simulator::Now();
		std::string simulation_start_string = simulation_start.ToStringiso();
		std::string simulation_end_string = simulation_end.ToStringiso();
		

		std::vector<ConnectionLink> infos = m_topology->GetSatellite2GroundConnectionLinkDetails();
		for(uint32_t i = 0; i < infos.size(); i++){
			ConnectionLink link = infos[i];
			uint32_t gndId = link.nodeid1;
			for(uint32_t j = 0; j < link.connectionTimeInterval.size(); j++){
				// for one link
				uint32_t satId = link.satnodes[j];
				string id = std::to_string(gndId) + "-to-" + std::to_string(satId);
				string name = std::to_string(gndId) + " to " + std::to_string(satId);
				string position1 = std::to_string(gndId)+"#position";
				string position2 = std::to_string(satId)+"#position";

				nlohmann::ordered_json jsonObject;
				jsonObject["id"] = id;
				jsonObject["name"] = name;

				// 创建 JSON 子对象 polyline
				nlohmann::ordered_json polyline;
				polyline["arcType"] = "NONE";
				polyline["width"] = 1;

				JulianDate start = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Seconds(link.connectionTimeInterval[j][0].first);
				JulianDate end = randomSatelliteNode->GetObject<SatellitePositionMobilityModel>()->GetStartTime() + Seconds(link.connectionTimeInterval[j][link.connectionTimeInterval[j].size()-1].second);
				std::string start_string = start.ToStringiso();
				std::string end_string = end.ToStringiso();

				nlohmann::ordered_json showArray;
				if(simulation_start_string != start_string )
				{
					std::string temp_time = simulation_start_string + '/' +start_string;
					nlohmann::ordered_json TempshowArray = {
						{ "interval", temp_time },
						{ "boolean", false }
					};
					showArray.push_back(TempshowArray);
				}

				for(uint32_t k = 0; k < link.connectionTimeInterval[j].size(); k++){
					std::pair<double, double> pair = link.connectionTimeInterval[j][k];
					auto curstart = simulation_start + Seconds(pair.first);
					auto curend = simulation_start + Seconds(pair.second);
					std::string time = curstart.ToStringiso()+'/' +curend.ToStringiso();

					nlohmann::ordered_json showArray_duation ={
						{ "interval", time },
						{ "boolean", true }
					};
					showArray.push_back(showArray_duation);

					if(k != link.connectionTimeInterval[j].size()-1)
					{
						auto curstart1 = simulation_start + Seconds(pair.second);
						auto curend1 = simulation_start + Seconds(link.connectionTimeInterval[j][k+1].first);
						std::string time1 = curstart1.ToStringiso()+'/' +curend1.ToStringiso();
						showArray_duation ={
							{ "interval", time1 },
							{ "boolean", false }
						};
						showArray.push_back(showArray_duation);
					}
				}

				if(end_string != simulation_end_string)
				{
					std::string temp_time = end_string + '/' +simulation_end_string;
					nlohmann::ordered_json TempshowArray = {
						{ "interval", temp_time },
						{ "boolean", false }
					};
					showArray.push_back(TempshowArray);

				}

				polyline["show"] = showArray;

				// 创建 JSON 子对象 material
				nlohmann::ordered_json material;
				material["solidColor"]["color"]["rgba"] = { 255, 255, 0, 255 };
				polyline["material"] = material;

				// 创建 JSON 子对象 positions
				nlohmann::ordered_json positions;
				positions["references"] = { position1, position2 };
				polyline["positions"] = positions;

				// 将 JSON 子对象 polyline 添加到父对象中
				jsonObject["polyline"] = polyline;

				json_v.push_back(jsonObject);

			}
		}


		std::ofstream HandoverResults(m_basicSimulation->GetRunDir() + "/results/cesium_results/handover_results.json", std::ofstream::out);
		if (HandoverResults.is_open()) {
			if(json_v.empty()){
				nlohmann::json data = nlohmann::json::array();
				HandoverResults << data.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				HandoverResults.close();
			}
			else{
				HandoverResults << json_v.dump(4);  // 使用缩进格式将 JSON 内容写入文件
				HandoverResults.close();
				//std::cout << "JSON file created successfully." << std::endl;
			}
		} else {
			std::cout << "Failed to create JSON file." << std::endl;
		}

	}


}
