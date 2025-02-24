#ifndef SIMULATOR_CONTRIB_SATELLITE_TOPOLOGY_MODEL_CPPJSON2STRUCTURE_HH_
#define SIMULATOR_CONTRIB_SATELLITE_TOPOLOGY_MODEL_CPPJSON2STRUCTURE_HH_

#include "json.hpp"
#include <iostream>
#include <fstream>
#include "ns3/vector.h"

using namespace std;
using json = nlohmann::json;
//extern std::string mpi_readin_dir_guid;

namespace jsonns {

	struct constellationinfo {
		std::string name;
		std::string walker_type;
		int number_of_planes;
		int number_of_sats_per_plane;
		int phase_factor;
		int altitude_km;
		int inclination_deg;
		int raan_deg;
		std::string propagator;
		std::string color;
	};

	inline void from_json(const json& j, constellationinfo& v) {
        j.at("name").get_to(v.name);
        j.at("walker_type").get_to(v.walker_type);
        j.at("number_of_planes").get_to(v.number_of_planes);
        j.at("number_of_sats_per_plane").get_to(v.number_of_sats_per_plane);
        j.at("phase_factor").get_to(v.phase_factor);
        j.at("altitude_km").get_to(v.altitude_km);
        j.at("inclination_deg").get_to(v.inclination_deg);
        j.at("raan_deg").get_to(v.raan_deg);
        j.at("propagator").get_to(v.propagator);
        j.at("color").get_to(v.color);
    }

	struct earthstationinfo {
		std::string id;
		std::string name;
		std::string network;
		std::string latitude;
		std::string longitude;
		std::string altitude;
	};

	inline void from_json(const json& j, earthstationinfo& v) {
        v.id = j.at("id").dump();
        v.name = j.at("name");
        v.network = j.at("network");
        v.latitude = j.at("latitude").dump();
        v.longitude = j.at("longitude").dump();
        v.altitude = j.at("altitude").dump();
    }

    struct basic_simulation_set_info {
        std::string epoch;
        std::string simulation_end_time_ns;
        std::string dynamic_state_update_interval_ns;
        std::string simulation_seed;
        std::string enable_sun_outage;
    };

    inline void from_json(const json& j, basic_simulation_set_info& v) {
    	v.epoch = j.at("epoch").dump();
    	v.simulation_end_time_ns = j.at("simulation_end_time_ns").dump();
    	v.dynamic_state_update_interval_ns = j.at("dynamic_state_update_interval_ns").dump();
    	v.simulation_seed = j.at("simulation_seed").dump();
    	v.enable_sun_outage = j.at("enable_sun_outage").dump();
    }

    struct basic_distributed_simulation_set_info {
        std::string enable_distributed;
        std::string distributed_simulator_implementation_type;
        std::string distributed_assign_algorithm;
    };

    inline void from_json(const json& j, basic_distributed_simulation_set_info& v) {
    	v.enable_distributed = j.at("enable_distributed").dump();
        j.at("distributed_simulator_implementation_type").get_to(v.distributed_simulator_implementation_type);
        j.at("distributed_assign_algorithm").get_to(v.distributed_assign_algorithm);
    }

//<! link
    struct csma_info {
    	std::string csma_enabled;
    };

    inline void from_json(const json& j, csma_info& v) {
    	v.csma_enabled = j.at("csma_enabled").dump();
    }

    struct aloha_info {
    	std::string aloha_enabled;
    };

    inline void from_json(const json& j, aloha_info& v) {
    	v.aloha_enabled = j.at("aloha_enabled").dump();
    }

    struct fdma_info {
    	std::string fdma_enabled;
    };

    inline void from_json(const json& j, fdma_info& v) {
    	v.fdma_enabled = j.at("fdma_enabled").dump();
    }

    struct feeder_link_info {
        std::string enable_gsl_data_rate_fixed;
        std::string gsl_data_rate_megabit_per_s;
        std::string gsl_max_queue_size_pkts;
				std::string gsl_error_rate_per_pkt;
        csma_info csma;
        aloha_info aloha;
        fdma_info fdma;
		std::string maximum_feeder_link_number;
		std::string enable_distance_nearest_first;
    };

    inline void from_json(const json& j, feeder_link_info& v) {
    	v.enable_gsl_data_rate_fixed = j.at("enable_gsl_data_rate_fixed").dump();
    	v.gsl_data_rate_megabit_per_s = j.at("gsl_data_rate_megabit_per_s").dump();
    	v.gsl_max_queue_size_pkts = j.at("gsl_max_queue_size_pkts").dump();
			v.gsl_error_rate_per_pkt = j.at("gsl_error_rate_per_pkt").dump();
        v.csma = j.at("csma");
        v.aloha = j.at("aloha");
        v.fdma = j.at("fdma");
        v.maximum_feeder_link_number = j.at("maximum_feeder_link_number").dump();
        v.enable_distance_nearest_first = j.at("enable_distance_nearest_first").dump();
    }

    struct ppp_info {
    	std::string ppp_enabled;
    };

    inline void from_json(const json& j, ppp_info& v) {
    	v.ppp_enabled = j.at("ppp_enabled").dump();
    }

    struct hdlc_info {
    	std::string hdlc_enabled;
    };

    inline void from_json(const json& j, hdlc_info& v) {
    	v.hdlc_enabled = j.at("hdlc_enabled").dump();
    }

    struct intersatellite_link_info {
        std::string enable_isl_data_rate_fixed;
        std::string isl_data_rate_megabit_per_s;
        std::string isl_max_queue_size_pkts;
				std::string isl_error_rate_per_pkt;
        ppp_info ppp;
        hdlc_info hdlc;
		std::string enable_grid_type_isl_establish;
    };

    inline void from_json(const json& j, intersatellite_link_info& v) {
    	v.enable_isl_data_rate_fixed = j.at("enable_isl_data_rate_fixed").dump();
    	v.isl_data_rate_megabit_per_s = j.at("isl_data_rate_megabit_per_s").dump();
    	v.isl_max_queue_size_pkts = j.at("isl_max_queue_size_pkts").dump();
			v.isl_error_rate_per_pkt = j.at("isl_error_rate_per_pkt").dump();
        v.ppp = j.at("ppp");
        v.hdlc = j.at("hdlc");
        v.enable_grid_type_isl_establish = j.at("enable_grid_type_isl_establish").dump();
    }

//<! ip
    struct ip_addressing {
        std::string enable_ipv4_addressing_protocol;
        std::string network_addressing_method;
        std::string network_part;
		std::string address_mask;
		std::string host_part_to_start_from;
    };

    inline void from_json(const json& j, ip_addressing& v) {
    	v.enable_ipv4_addressing_protocol = j.at("enable_ipv4_addressing_protocol").dump();
        j.at("network_addressing_method").get_to(v.network_addressing_method);
        j.at("network_part").get_to(v.network_part);
        j.at("address_mask").get_to(v.address_mask);
        j.at("host_part_to_start_from").get_to(v.host_part_to_start_from);
    }

    struct ip_export_routing_tables_info {
    	std::string enable_export_routing_tables;
		std::string time_interval_s;
		std::string target_node;
    };

    inline void from_json(const json& j, ip_export_routing_tables_info& v) {
    	v.enable_export_routing_tables = j.at("enable_export_routing_tables").dump();
    	v.time_interval_s = j.at("time_interval_s").dump();
        j.at("target_node").get_to(v.target_node);
    }

    struct ip_ospf_info {
    	std::string ospf_enabled;
    	int16_t priority;
		std::string installation_scope;
		bool prompt_mode;

		uint32_t hello_interval_s;
		uint32_t router_dead_interval_s;
		uint32_t retransmit_interval_s;
		uint32_t LSRefreshTime_s;
    };

    inline void from_json(const json& j, ip_ospf_info& v) {
    	v.ospf_enabled = j.at("ospf_enabled").dump();
    	v.priority = j.at("priority");
        j.at("installation_scope").get_to(v.installation_scope);
        v.prompt_mode = j.at("prompt_mode");

        v.hello_interval_s = j.at("hello_interval_s");
        v.router_dead_interval_s = j.at("router_dead_interval_s");
        v.retransmit_interval_s = j.at("retransmit_interval_s");
        v.LSRefreshTime_s = j.at("LSRefreshTime_s");
    }

    struct ip_aodv_info {
    	std::string aodv_enabled;
		std::string priority;
		std::string installation_scope;
    };

    inline void from_json(const json& j, ip_aodv_info& v) {
    	v.aodv_enabled = j.at("aodv_enabled").dump();
    	v.priority = j.at("priority").dump();
        j.at("installation_scope").get_to(v.installation_scope);
    }

    struct ip_minhop_info {
    	std::string minhop_enabled;
		std::string priority;
		std::string installation_scope;
    };

    inline void from_json(const json& j, ip_minhop_info& v) {
    	v.minhop_enabled = j.at("minhop_enabled").dump();
    	v.priority = j.at("priority").dump();
        j.at("installation_scope").get_to(v.installation_scope);
    }

    struct ip_bgp_info {
    	std::string bgp_enabled;
		std::string priority;
		std::string installation_scope;
    };

    inline void from_json(const json& j, ip_bgp_info& v) {
    	v.bgp_enabled = j.at("bgp_enabled").dump();
    	v.priority = j.at("priority").dump();
        j.at("installation_scope").get_to(v.installation_scope);
    }

    struct ip_satellite_to_ground_routing_info {
    	std::string satellite_to_ground_routing_enabled;
		std::string priority;
		std::string installation_scope;
    };

    inline void from_json(const json& j, ip_satellite_to_ground_routing_info& v) {
    	v.satellite_to_ground_routing_enabled = j.at("satellite_to_ground_routing_enabled").dump();
    	v.priority = j.at("priority").dump();
        j.at("installation_scope").get_to(v.installation_scope);
    }

    struct ip_routing {
    	ip_export_routing_tables_info export_routing_tables;
    	ip_ospf_info ospf;
    	ip_minhop_info minhop;
    	ip_aodv_info aodv;
    	ip_bgp_info bgp;
    	ip_satellite_to_ground_routing_info satellite_to_ground_routing;
    };

    inline void from_json(const json& j, ip_routing& v) {
        v.export_routing_tables = j.at("export_routing_tables");
        v.ospf = j.at("ospf");
        v.aodv = j.at("aodv");
        v.minhop = j.at("minhop");
        v.bgp = j.at("bgp");
        v.satellite_to_ground_routing = j.at("satellite_to_ground_routing");
    }


    //<! physical
	//xhqin
	struct antenna {
		double minimum_elevation_angle_deg;
		double frequency;
		double transmit_power;
		double transmit_antenna_gain;
		std::string scenario;
		bool enable_ber;
	};

	inline void from_json(const json& j, antenna& v) {
		v.minimum_elevation_angle_deg = j.at("minimum_elevation_angle_deg");
		v.frequency = j.at("frequency_hz");
		v.transmit_power = j.at("transmit_power_dbm");
		v.transmit_antenna_gain = j.at("transmit_antenna_gain_dbi");
		v.scenario = j.at("scenario").dump();
		v.enable_ber = j.at("enable_ber");
	}

	struct dvbs2_info {
		std::string enable_DVBS2_protocol;
		std::string MCS;
	};
	inline void from_json(const json& j, dvbs2_info& v) {
		v.enable_DVBS2_protocol = j.at("enable_DVB-S2_protocol").dump();
		v.MCS = j.at("MCS").dump();
	}
	struct dvbs2x_info {
		std::string enable_DVBS2X_protocol;
		std::string MCS;
	};
	inline void from_json(const json& j, dvbs2x_info& v) {
		v.enable_DVBS2X_protocol = j.at("enable_DVB-S2X_protocol").dump();
		v.MCS = j.at("MCS").dump();
	}
	struct dvbrcs2_info {
		//std::string enable_DVBRCS2_protocol;
		std::string Waveform;
	};
	inline void from_json(const json& j, dvbrcs2_info& v) {
		//v.enable_DVBRCS2_protocol = j.at("enable_DVB-RCS2_protocol").dump();
		v.Waveform = j.at("Waveform").dump();
	}


//<! application
    struct application {
    	std::string enable_application_scheduler_udp;
    	std::string enable_application_scheduler_tcp;
    	std::string enable_application_scheduler_rtp;
    	std::string enable_application_scheduler_3gpp_http;
    	std::string enable_application_scheduler_ftp;

    };

    inline void from_json(const json& j, application& v) {
        v.enable_application_scheduler_udp = j.at("enable_application_scheduler_udp").dump();
        v.enable_application_scheduler_tcp = j.at("enable_application_scheduler_tcp").dump();
        v.enable_application_scheduler_rtp = j.at("enable_application_scheduler_rtp").dump();
        v.enable_application_scheduler_3gpp_http = j.at("enable_application_scheduler_3gpp_http").dump();
        v.enable_application_scheduler_ftp = j.at("enable_application_scheduler_ftp").dump();
    }

//<! tracing.json
    struct target_device_info {
    	std::string pairs;
    };

    inline void from_json(const json& j, target_device_info& v) {
        v.pairs = j;
    }

    struct utilization {
    	std::string link_utilization_tracing_enabled;
    	std::string target_device_all;
    	target_device_info target_device[50];
    	std::string link_utilization_tracing_interval_ns;
    	std::string enable_trajectory_tracing;
    };

    inline void from_json(const json& j, utilization& v) {
        v.link_utilization_tracing_enabled = j.at("link_utilization_tracing_enabled").dump();
        v.target_device_all = j.at("target_device_all").dump();
        int tilength = j["target_device"].size();
		for (int i = 0; i < tilength; i++) {
			v.target_device[i] = j["target_device"][i];
		}
        v.link_utilization_tracing_interval_ns = j.at("link_utilization_tracing_interval_ns").dump();
        v.enable_trajectory_tracing = j.at("enable_trajectory_tracing").dump();

    }

    struct wireshark {
    	std::string wireshark_enabled;
    	std::string wireshark_target_device_all;
    	target_device_info target_device[50];
    };

    inline void from_json(const json& j, wireshark& v) {
        v.wireshark_enabled = j.at("wireshark_enabled").dump();
        v.wireshark_target_device_all = j.at("wireshark_target_device_all").dump();
        int tilength = j["wireshark_target_device"].size();
		for (int i = 0; i < tilength; i++) {
			v.target_device[i] = j["wireshark_target_device"][i];
		}
    }

//<! access.json
    // object_id_access_for
    struct object_id_access_for {
        std::string my_object_id_access_for;
	};

	inline void from_json(const json& j, object_id_access_for& v) {
		v.my_object_id_access_for = j["object_id_access_for"].dump();
	}

	// access_object_ids
    struct access_object_id {
        std::string one_access_object_id;
	};

	inline void from_json(const json& j, access_object_id& v) {
		v.one_access_object_id = j.dump();
	}

    struct access_object_ids {
    	std::vector<access_object_id> my_access_object_ids;
	};

	inline void from_json(const json& j, access_object_ids& v) {
		int tilength = j["access_object_ids"].size();
		for (int i = 0; i < tilength; i++) {
			v.my_access_object_ids.push_back(j["access_object_ids"][i]);
		}
	}

	// time
    struct specify_time_period {
        std::string start;
        std::string stop;
	};

	inline void from_json(const json& j, specify_time_period& v) {
		v.start = j.at("start").dump();
		v.stop = j.at("stop").dump();
	}

    struct access_time {
    	std::string use_scenario_time_period;
    	specify_time_period my_specify_time_period;
    	std::string time_interval;
	};

	inline void from_json(const json& j, access_time& v) {
		v.use_scenario_time_period = j.at("use_scenario_time_period").dump();
		v.my_specify_time_period = j.at("specify_time_period");
		v.time_interval = j.at("time_interval_ns").dump();
	}

	// output
    struct access_report {
        std::string access;
        std::string aer;
	};

	inline void from_json(const json& j, access_report& v) {
		v.access = j.at("access").dump();
		v.aer = j.at("aer").dump();
	}

    struct access_graphs {
        std::string access;
        std::string aer;
	};

	inline void from_json(const json& j, access_graphs& v) {
		v.access = j.at("access").dump();
		v.aer = j.at("aer").dump();
	}

    struct access_output {
    	access_report my_report;
    	access_graphs my_graphs;
	};

	inline void from_json(const json& j, access_output& v) {
		v.my_report = j.at("report");
		v.my_graphs = j.at("graphs");
	}

	// aer_results
    struct single_aer_struct {
    	double cur_time;
    	std::string cur_time_str;
    	double azimuth;
    	double elevation;
    	double range;
	};

    struct aer_struct {
    	int object_access_for;
    	int object_access;
    	std::vector<single_aer_struct> aers;
	};

    // access_results
    struct single_access_struct {
    	int64_t start_ns;
    	int64_t stop_ns;
    	std::string start_str;
    	std::string stop_str;
    	bool finished = false;
	};

    struct access_struct {
    	uint32_t object_access_for;
    	uint32_t object_access;
    	std::vector<single_access_struct> accesses;
	};


//<! coverage.json
	// object_id_coverage_for
	struct object_id_coverage_for {
		std::string my_object_id_coverage_for;
	};

	inline void from_json(const json &j, object_id_coverage_for &v) {
		v.my_object_id_coverage_for = j["object_id_coverage_for"].dump();
	}

	// coverage_object_ids
	struct coverage_object_id {
		std::string one_coverage_object_id;
	};

	inline void from_json(const json &j, coverage_object_id &v) {
		v.one_coverage_object_id = j.dump();                 //?? useless??x
	}

	struct coverage_object_ids {
		std::vector<coverage_object_id> my_coverage_object_ids;
	};

	inline void from_json(const json &j, coverage_object_ids &v) {
		int tilength = j["coverage_object_ids"].size();
		coverage_object_id tmp;
		for (int i = 0; i < tilength; i++) {
			tmp.one_coverage_object_id = j["coverage_object_ids"][i].dump();
			v.my_coverage_object_ids.push_back(tmp);
		}
	}

	// time
	struct cov_specify_time_period {
		std::string start;
		std::string stop;
	};

	inline void from_json(const json &j, cov_specify_time_period &v) {
		v.start = j.at("start").dump();
		v.stop = j.at("stop").dump();
	}

	struct coverage_time {
		std::string use_scenario_time_period;
		cov_specify_time_period my_cov_specify_time_period;
		std::string time_interval;
	};

	inline void from_json(const json &j, coverage_time &v) {
		v.use_scenario_time_period = j.at("use_scenario_time_period").dump();
		v.my_cov_specify_time_period = j.at("specify_time_period");
		v.time_interval = j.at("time_interval_ns").dump();
	}

	//fom of value
	struct simple_coverage{
		std::string simple_coverage_enabled;
	};

	struct access_duration{
		std::string access_duration_enabled;
	};

	struct fom_value{
		simple_coverage my_simple_coverage;
		access_duration my_access_duration;
	};

	inline void from_json(const json &j, fom_value &v){
		v.my_simple_coverage.simple_coverage_enabled = j.at("simple_coverage").dump();
		v.my_access_duration.access_duration_enabled = j.at("access_duration").dump();
	}

	// output
	struct coverage_report {
		std::string coverage;
		std::string fom_value;
	};

	inline void from_json(const json &j, coverage_report &v) {
		v.coverage = j.at("coverage").dump();
		v.fom_value = j.at("fom_value").dump();
	}

	struct coverage_graphs {
		std::string coverage;
		std::string fom_value;
	};

	inline void from_json(const json &j, coverage_graphs &v) {
		v.coverage = j.at("coverage").dump();
		v.fom_value = j.at("fom_value").dump();
	}

	struct coverage_output {
		coverage_report my_report;
		coverage_graphs my_graphs;
	};

	inline void from_json(const json &j, coverage_output &v) {
		v.my_report = j.at("report");
		v.my_graphs = j.at("graphs");
	}//no dump

	// simple_coverage_results
	struct simple_coverge_struct{
		int64_t time_ns;
		std::string time_str;
		int64_t value;
	};

	// cov_access_results
	struct cov_single_access_struct {
		uint32_t satellite_number;
		uint32_t sort_number;
		int64_t start_ns;
		int64_t stop_ns;
		std::string start_str;
		std::string stop_str;
		bool finished = false;
	};

	struct cov_access_struct {
		uint32_t object_access_for;
		uint32_t object_access;
		std::vector<cov_single_access_struct> accesses;
	};

	//number of satellites above
	struct satellite_above_struct {
		int64_t time_ns;
		std::string time_str;
		int32_t value;
	};




//<! application_schedule_udp/tcp/rtp.json
    struct application_schedule {
        std::string flow_id;
        std::string sender;
        std::string receiver;
        std::string target_flow_rate_mbps;
        std::string start_time_ns;
        std::string duration_time_ns;
        std::string code_type;
        std::string service_type;
	};

	inline void from_json(const json& j, application_schedule& v) {
		v.flow_id = j["flow_id"].dump();
		v.sender = j["sender"].dump();
		v.receiver = j["receiver"].dump();
		v.target_flow_rate_mbps = j["target_flow_rate_mbps"].dump();
		v.start_time_ns = j["start_time_ns"].dump();
		v.duration_time_ns = j["duration_time_ns"].dump();
		v.code_type = j["code_type"].dump();
		v.service_type = j["service_type"].dump();
	}

    struct application_schedules {
        std::vector<application_schedule> my_application_schedules;
	};

	inline void from_json(const json& j, application_schedules& v) {
		int tilength = j.size();
		for (int i = 0; i < tilength; i++) {
			v.my_application_schedules.push_back(j[i]);
		}
	}


//<! application_schedule_3gpphttp.json
    struct application_schedule_3gpphttp {
        std::string flow_id;
        std::string sender;
        std::string receiver;
        std::string start_time_ns;
        std::string duration_time_ns;
	};

	inline void from_json(const json& j, application_schedule_3gpphttp& v) {
		v.flow_id = j["flow_id"].dump();
		v.sender = j["sender"].dump();
		v.receiver = j["receiver"].dump();
		v.start_time_ns = j["start_time_ns"].dump();
		v.duration_time_ns = j["duration_time_ns"].dump();
	}

    struct application_schedules_3gpphttp {
        std::vector<application_schedule_3gpphttp> my_application_schedules;
	};

	inline void from_json(const json& j, application_schedules_3gpphttp& v) {
		int tilength = j.size();
		for (int i = 0; i < tilength; i++) {
			v.my_application_schedules.push_back(j[i]);
		}
	}


//<! application_schedule_ftp.json
	struct application_schedule_ftp {
		std::string flow_id;
		std::string sender;
		std::string receiver;
		std::string start_time_ns;
		std::string size_bytes;
	};

	inline void from_json(const json& j, application_schedule_ftp& v) {
		v.flow_id = j["flow_id"].dump();
		v.sender = j["sender"].dump();
		v.receiver = j["receiver"].dump();
		v.start_time_ns = j["start_time_ns"].dump();
		v.size_bytes = j["size_bytes"].dump();
	}

	struct application_schedules_ftp {
		std::vector<application_schedule_ftp> my_application_schedules;
	};

	inline void from_json(const json& j, application_schedules_ftp& v) {
		int tilength = j.size();
		for (int i = 0; i < tilength; i++) {
			v.my_application_schedules.push_back(j[i]);
		}
	}

//<! sun_trajectory (eci_cspice).json
	struct sun_trajectory_content {
		int64_t time_stamp_ns;
		ns3::Vector3D position_km;
		ns3::Vector3D velocity_kmps;
	};

	inline void from_json(const json& j, sun_trajectory_content& v) {
		v.time_stamp_ns = j["time_stamp_ns"];
		v.position_km.x = j["position_km"][0];
		v.position_km.y = j["position_km"][1];
		v.position_km.z = j["position_km"][2];
		v.velocity_kmps.x = j["velocity_km/s"][0];
		v.velocity_kmps.y = j["velocity_km/s"][1];
		v.velocity_kmps.z = j["velocity_km/s"][2];

	}





}


#endif /* SIMULATOR_CONTRIB_SATELLITE_TOPOLOGY_MODEL_CPPJSON2STRUCTURE_HH_ */
