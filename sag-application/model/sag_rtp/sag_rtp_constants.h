#ifndef SAG_RTP_CONSTANTS_H
#define SAG_RTP_CONSTANTS_H

#include <stdint.h>
#include <stddef.h>
#include "ns3/ospf-lsa-identifier.h"

const uint32_t DEFAULT_PACKET_SIZE = 1380; // 1500 (point-to-point default) - 20 (IP) - 8 (UDP) = 1472, 1000
const uint32_t DEFAULT_PACKET_SIZE_UDP = 1472; // 1500 (point-to-point default) - 20 (IP) - 8 (UDP) = 1472, 1000
const uint32_t IPV4_HEADER_SIZE = 20;
const uint32_t UDP_HEADER_SIZE = 8;
const uint32_t IPV4_UDP_OVERHEAD = IPV4_HEADER_SIZE + UDP_HEADER_SIZE;
const uint64_t SAG_RTP_FEEDBACK_PERIOD_US = 100 * 1000 * 1000;
extern bool Topology_CHANGE_GSL;
extern bool Topology_CHANGE_ISL;

// syncodec parameters
const uint32_t SYNCODEC_DEFAULT_FPS = 30;
enum SyncodecType {
    SYNCODEC_TYPE_PERFECT = 0,
    SYNCODEC_TYPE_FIXFPS,
    SYNCODEC_TYPE_STATS,
    SYNCODEC_TYPE_TRACE,
    SYNCODEC_TYPE_SHARING,
    SYNCODEC_TYPE_HYBRID,
	SYNCODEC_TYPE_INVALID
};

enum TraceType {
    TRACE_TYPE_CHAT = 0,
    TRACE_TYPE_BBB,
    TRACE_TYPE_CONCAT,
    TRACE_TYPE_ED,
    TRACE_TYPE_FOREMAN,
    TRACE_TYPE_NEWS,
	TRACE_TYPE_INVALID
};

struct hash_ospfIdt {//公有成员
    size_t operator()(const ns3::ospf::OSPFLinkStateIdentifier& p) const {//别忘记const
        return std::hash<int>()(p.m_LinkStateID.Get());
    }
};
struct equal_ospfIdt{
    bool operator()(const ns3::ospf::OSPFLinkStateIdentifier&a,const ns3::ospf::OSPFLinkStateIdentifier&b) const{
        return a.m_LinkStateID==b.m_LinkStateID;
    }
};

struct hash_adr {//公有成员
	size_t operator()(const ns3::Ipv4Address& p) const {//别忘记const
        return std::hash<int>()(p.Get());
    }
};
struct equal_adr{
    bool operator()(const ns3::Ipv4Address&a,const ns3::Ipv4Address&b) const{
        return a.Get()==b.Get();
    }
};

/**
 * Parameters for the rate shaping buffer as specified in draft-ietf-rtp-nada
 * These are the default values according to the draft
 * The rate shaping buffer is currently implemented in the sender ns3
 * application (#ns3::SAGApplicationLayerRTP). For other congestion controllers
 * that do not need the rate shaping buffer, you can disable it by
 * setting USE_BUFFER to false.
 */
const bool USE_BUFFER = true;
const float BETA_V = 0.0; // 0.0
const float BETA_S = 0.0; // 0.0
const uint32_t MAX_QUEUE_SIZE_SANITY = 8* 10 * 1000 * 1000; //bytes 10M

/* topology parameters */
const uint32_t T_MAX_S = 500;  // maximum simulation duration  in seconds
const double T_TCP_LOG = 2;  // sample interval for log TCP flows

/* Default topology setting parameters */
/*
const uint32_t WIFI_TOPO_MACQUEUE_MAXNPKTS = 1000;
const uint32_t WIFI_TOPO_ARPCACHE_ALIVE_TIMEOUT = 24 * 60 * 60; // 24 hours
const float WIFI_TOPO_2_4GHZ_PATHLOSS_EXPONENT = 3.0f;
const float WIFI_TOPO_2_4GHZ_PATHLOSS_REFLOSS = 40.0459f;
const uint32_t WIFI_TOPO_CHANNEL_WIDTH = 20;   // default channel width: 20MHz
*/

#endif /* SAG_RTP_CONSTANTS_H */
