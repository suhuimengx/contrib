#include "ns3/quic-optimizer.h"

namespace ns3 {

void QuicOptimizer::Generic(std::string filename) {

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;

		    // Initial congestion window
		    uint32_t init_cwnd_pkts = 10;  // 1 is default, but we use 10
		    init_cwnd_pkts = j["quic"]["initial_slow_start_threshold"];
		    printf("  > Initial CWND............... %u packets\n", init_cwnd_pkts);
		    Config::SetDefault("ns3::QuicSocketBase::InitialSlowStartThreshold", UintegerValue(init_cwnd_pkts));

		    // Send buffer size
		    int64_t snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    snd_buf_size_byte = j["quic"]["send_buffer_size_bytes"];
		    printf("  > Send buffer size........... %.3f MB\n", snd_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::QuicSocketBase::SocketSndBufSize", UintegerValue(snd_buf_size_byte));

		    // Stream send buffer size
		    int64_t stream_snd_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    stream_snd_buf_size_byte = j["quic"]["stream_send_buffer_size_bytes"];
		    printf("  > Send buffer size........... %.3f MB\n", snd_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::QuicStreamBase::StreamSndBufSize", UintegerValue(stream_snd_buf_size_byte));

		    // Receive buffer size
		    int64_t rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    rcv_buf_size_byte =  j["quic"]["receive_buffer_size_bytes"];
		    printf("  > Receive buffer size........ %.3f MB\n", rcv_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::QuicSocketBase::SocketRcvBufSize", UintegerValue(rcv_buf_size_byte));

		    // Stream send buffer size
		    int64_t stream_rcv_buf_size_byte = 131072 * 256;  // 131072 bytes = 128 KiB is default, we set to 32 MiB
		    stream_rcv_buf_size_byte = j["quic"]["stream_receive_buffer_size_bytes"];
		    printf("  > Send buffer size........... %.3f MB\n", snd_buf_size_byte / 1e6);
		    Config::SetDefault("ns3::QuicStreamBase::StreamRcvBufSize", UintegerValue(stream_rcv_buf_size_byte));

		    // Segment size
		    int64_t segment_size_byte = 1460;   // 536 byte is default, but we know the a point-to-point network device has an MTU of 1500.
		    segment_size_byte = j["quic"]["max_packet_byte"];
		    printf("  > Segment size............... %" PRId64 " byte\n", segment_size_byte);
		    Config::SetDefault("ns3::QuicSocketBase::MaxPacketSize", UintegerValue(segment_size_byte));

		    // Pacing
		    bool pacing_enabled = false;  // Default: false.
		    pacing_enabled = j["quic"]["pacing"];
		    printf("  > Pacing..................... %s\n", pacing_enabled ? "enabled" : "disabled");
		    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(pacing_enabled));

            //Max Pacing Rate
					if(pacing_enabled){
							uint32_t pacing_rate = 4;
							pacing_rate = j["quic"]["max_pacing_rate"];
							string max_pacing_rate = std::to_string(pacing_rate) + "Gbps";
							printf("  > Max Pacing Rate..................... %s\n", max_pacing_rate.c_str());
							Config::SetDefault ("ns3::TcpSocketState::MaxPacingRate", StringValue (max_pacing_rate));
					}

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

		jfile.close();
	}

}

void QuicOptimizer::OptimizeBasic(Ptr<BasicSimulation> basicSimulation) {
    std::cout << "QUIC OPTIMIZATION BASIC" << std::endl;
    std::string filename = basicSimulation->GetRunDir() + "/config_protocol/transport_global_attribute.json";
    Generic(filename);
    std::cout << std::endl;
    basicSimulation->RegisterTimestamp("Setup Quic parameters");
}


}
