#include "scps-tp-optimizer.h"

namespace ns3 {

void ScpsTpOptimizer::Generic(std::string filename) {

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
		    // Clock granularity
		    int64_t clockGranularity_ns = 1;
		    clockGranularity_ns = j["scps-tp"]["clock_granularity_ns"];
		    printf("  > Clock granularity............... %" PRId64 " ns\n", clockGranularity_ns);
		    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(NanoSeconds(clockGranularity_ns)));

		    // Initial congestion window
		    uint32_t init_cwnd_pkts = 10;  // 1 is default, but we use 10
		    init_cwnd_pkts = j["scps-tp"]["initial_CWND_packets"];
		    printf("  > Initial CWND............... %u packets\n", init_cwnd_pkts);
		    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(init_cwnd_pkts));

		    // Send buffer size
		    int64_t snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    snd_buf_size_byte = j["scps-tp"]["send_buffer_size_bytes"];
		    printf("  > Send buffer size........... %.3f MB\n", snd_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(snd_buf_size_byte));

		    // Receive buffer size
		    int64_t rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    rcv_buf_size_byte =  j["scps-tp"]["receive_buffer_size_bytes"];
		    printf("  > Receive buffer size........ %.3f MB\n", rcv_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(rcv_buf_size_byte));

		    // Segment size
		    int64_t segment_size_byte = 1380;   // 536 byte is default, but we know the a point-to-point network device has an MTU of 1500.
		    segment_size_byte = j["scps-tp"]["segment_size_byte"];
		    // IP header size: min. 20 byte, max. 60 byte
		    // TCP header size: min. 20 byte, max. 60 byte
		    // So, 1500 - 60 - 60 = 1380 would be the safest bet (given we don't do tunneling)
		    // This could be increased higher, e.g. as discussed here:
		    // https://blog.cloudflare.com/increasing-ipv6-mtu/ (retrieved April 7th, 2020)
		    // In past ns-3 simulations, I've observed that the IP + TCP header is generally not larger than 80 bytes.
		    // This means it could be potentially set closer to 1400-1420.
		    printf("  > Segment size............... %" PRId64 " byte\n", segment_size_byte);
		    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segment_size_byte));

		    // Timestamp option
		    bool opt_timestamp_enabled = true;  // Default: true.
		    opt_timestamp_enabled = j["scps-tp"]["timestamp_option"];
		    // To get an RTT measurement with a resolution of less than 1ms, it needs
		    // to be disabled because the fields in the TCP Option are in milliseconds.
		    // When disabling it, there are two downsides:
		    //  (1) Less protection against wrapped sequence numbers (PAWS)
		    //  (2) Algorithm to see if it has entered loss recovery unnecessarily are not as possible (Eiffel)
		    // See: https://tools.ietf.org/html/rfc7323#section-3
		    //      https://tools.ietf.org/html/rfc3522
		    printf("  > Timestamp option........... %s\n", opt_timestamp_enabled ? "enabled" : "disabled");
		    Config::SetDefault("ns3::TcpSocketBase::Timestamp", BooleanValue(opt_timestamp_enabled));

		    // SNACK option
		    bool opt_snack_enabled = true;  // Default: true.
		    opt_snack_enabled = j["scps-tp"]["SNACK_option"];
		    // Selective negative acknowledgment improves SCPSTP performance, however can
		    // be time-consuming to simulate because set operations and merges have
		    // to be performed.
		    printf("  > SNACK option................ %s\n", opt_snack_enabled ? "enabled" : "disabled");
		    //Config::SetDefault("ns3::TcpSocketBase::Snack", BooleanValue(opt_snack_enabled));

		    // Window scaling option
		    bool opt_win_scaling_enabled = true;  // Default: true.
		    opt_win_scaling_enabled = j["scps-tp"]["window_scaling_option"];
		    printf("  > Window scaling option...... %s\n", opt_win_scaling_enabled ? "enabled" : "disabled");
		    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(opt_win_scaling_enabled));

		    // Pacing
		    bool pacing_enabled = true;  // Default: true.
		    pacing_enabled = j["scps-tp"]["pacing"];
		    printf("  > Pacing..................... %s\n", pacing_enabled ? "enabled" : "disabled");
		    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(pacing_enabled));

        //Max Pacing Rate(与Quic重复)
        if(pacing_enabled){
          uint32_t pacing_rate = 5;
          pacing_rate = j["scps-tp"]["max_pacing_rate"];
          string max_pacing_rate = std::to_string(pacing_rate) + "Mbps";
          printf("  > Max Pacing Rate..................... %s\n", max_pacing_rate.c_str());
          Config::SetDefault ("ns3::TcpSocketState::MaxPacingRate", StringValue (max_pacing_rate));
        }

        //ssthresh
        uint32_t ssthresh = 4294967295; // Default: 4294967295(maximum value)
        ssthresh = j["scps-tp"]["initial_slow_start_threshold"];
        printf("  > Initial Slow Start Threshold..................... %u\n", ssthresh);
        Config::SetDefault("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue(ssthresh));

        //ECN
        bool ecn_enabled = true;  // Default: true.
        ecn_enabled = j["scps-tp"]["ecn"];
        EnumValue ecn;
        if(ecn_enabled){
          ecn = EnumValue(ns3::TcpSocketState::On);
        }
        else if(!ecn_enabled){
          ecn = EnumValue(ns3::TcpSocketState::Off);
        }
        printf("  > ECN..................... %s\n", ecn_enabled ? "enabled" : "disabled");
        Config::SetDefault("ns3::TcpSocketBase::UseEcn", ecn);

        //进入链路中断的最大重传次数DataRetries
        uint16_t dataRetries = 4;
        dataRetries = j["scps-tp"]["data_retries"];
        printf("  > Data Retries..................... %u\n", dataRetries);
        Config::SetDefault("ns3::TcpSocket::DataRetries", UintegerValue(dataRetries));

        //链路中断状态下的最大重传探测次数DataRetriesForLinkOut
        uint16_t dataRetriesForLinkOut = 6;
        dataRetriesForLinkOut = j["scps-tp"]["data_retries_for_link_out"];
        printf("  > Data Retries For Link Out..................... %u\n", dataRetriesForLinkOut);
        Config::SetDefault("ns3::ScpsTpSocketBase::DataRetriesForLinkOut", UintegerValue(dataRetriesForLinkOut));

        //固定ACK频率
        uint16_t delAckTimeOut = 50; // 50ms is default, 0 is disabled
        delAckTimeOut = j["scps-tp"]["del_ack_timeout_ms"];
        printf("  > Delay ACK Timeout..................... %u ms\n", delAckTimeOut);
        if(delAckTimeOut != 0){
          Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(100000));
          Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(MilliSeconds(delAckTimeOut)));
        }


		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();
	}

}

void ScpsTpOptimizer::OptimizeBasic(Ptr<BasicSimulation> basicSimulation) {
    std::cout << "SCPSTP OPTIMIZATION BASIC" << std::endl;
    std::string filename = basicSimulation->GetRunDir() + "/config_protocol/transport_global_attribute.json";
    Generic(filename);
    std::cout << std::endl;
    basicSimulation->RegisterTimestamp("Setup SCPSTP parameters");
}

}
