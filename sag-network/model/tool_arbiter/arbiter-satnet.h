/*
 * Copyright (c) 2020 ETH Zurich
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

#ifndef ARBITER_SATNET_H
#define ARBITER_SATNET_H

#include <map>
// #include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <tuple>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include <functional>
//#include "ns3/topology-satellite-network.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-header.h"
#include "ns3/arbiter.h"

namespace ns3 {

struct IPv6AddressBuf
{
    uint8_t buf[16];

    IPv6AddressBuf(){
        memset(buf, 0 , sizeof(buf));
    }

    IPv6AddressBuf(const uint8_t initialBuf[16]) {
        memcpy(buf, initialBuf, sizeof(buf));
    }

    IPv6AddressBuf(uint8_t ipv6[16]){
        memcpy(buf, ipv6, 16);
    }

    IPv6AddressBuf(const IPv6AddressBuf& other) {
        memcpy(buf, other.buf, sizeof(buf));
    }

    IPv6AddressBuf& operator=(const IPv6AddressBuf& other) {
        if (this == &other) {
            return *this; // 自赋值检查
        }

        memcpy(buf, other.buf, sizeof(buf));
        return *this;
    }

    IPv6AddressBuf& operator=(const uint8_t ipv6Buf[16]) {
        memcpy(buf, ipv6Buf, sizeof(buf));
        return *this;
    }

    bool operator < (const IPv6AddressBuf& other) const{
        for (int i = 0; i < 16; ++i) {
            if (buf[i] < other.buf[i]) {
                return true;  
            } else if (buf[i] > other.buf[i]) {
                return false; 
            }
        }
        return false;
    }

    bool compare(IPv6AddressBuf& other)
    {
        return *this < other;
    }
};
struct IPv6AddressBufComparator {
    bool operator()(IPv6AddressBuf addr1, IPv6AddressBuf addr2) const { 
        return  addr1 < addr2;
    }
};

class ArbiterSatnet : public Arbiter
{

public:
    static TypeId GetTypeId (void);
    ArbiterSatnet();
    ArbiterSatnet(Ptr<Node> this_node, NodeContainer nodes);
    void DoInitialize(Ptr<Node> this_node, NodeContainer nodes);
    void SetIpAddresstoNodeId(std::unordered_map<uint32_t, uint32_t>& Ip2NodeId);

    uint32_t ResolveNodeIdFromIp(uint32_t ip);
    /**
     * Base decide how to forward. Directly called by ipv4-arbiter-routing.
     * It does some nice pre-processing and checking and calls Decide() of the
     * subclass to actually make the decision.
     *
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     *
     * @return Routing arbiter result.
     */
    ArbiterResultIPv4 BaseDecide(
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader
    );

    // Topology implementation
    ArbiterResultIPv4 Decide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    );

    /**
     * Decide where the packet needs to be routed to.
     *
     * @param source_node_id                                Node where the packet originated from
     * @param target_node_id                                Node where the packet has to go to
     * @param neighbor_node_ids                             All neighboring nodes from which to choose
     * @param pkt                                           Packet
     * @param ipHeader                                      IP header instance
     * @param is_socket_request_for_source_ip               True iff it is a request for a source IP address,
     *                                                      as such the returning next hop is only used to get the
     *                                                      interface IP address
     *
     * @return Tuple of (next node id, my own interface id, next interface id)
     */
    virtual std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    ) = 0;

    void SetIpv6AddresstoNodeId(std::map<IPv6AddressBuf, uint32_t, IPv6AddressBufComparator>& Ipv62NodeId);

    uint32_t ResolveNodeIdFromIp(uint8_t ipv6[16]);
    /**
     * Base decide how to forward. Directly called by ipv6-arbiter-routing.
     * It does some nice pre-processing and checking and calls Decide() of the
     * subclass to actually make the decision.
     *
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     *
     * @return Routing arbiter result.
     */
    ArbiterResultIPv6 BaseDecide(
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv6Header const &ipHeader
    );

    // Topology implementation
    ArbiterResultIPv6 Decide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv6Header const &ipHeader,
            bool is_socket_request_for_source_ip
    );

    /**
     * Decide where the packet needs to be routed to.
     *
     * @param source_node_id                                Node where the packet originated from
     * @param target_node_id                                Node where the packet has to go to
     * @param neighbor_node_ids                             All neighboring nodes from which to choose
     * @param pkt                                           Packet
     * @param ipHeader                                      IP header instance
     * @param is_socket_request_for_source_ip               True iff it is a request for a source IP address,
     *                                                      as such the returning next hop is only used to get the
     *                                                      interface IP address
     *
     * @return Tuple of (next node id, my own interface id, next interface id)
     */
    virtual std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv6Header const &ipHeader,
            bool is_socket_request_for_source_ip
    ) = 0;

    virtual std::string StringReprOfForwardingState() = 0;

protected:
    int32_t m_node_id;
    ns3::NodeContainer m_nodes;

private:

    std::unordered_map<uint32_t, uint32_t> m_ip_to_node_id;

    std::map<IPv6AddressBuf, uint32_t, IPv6AddressBufComparator> m_ipv6_to_node_id;
};

}

#endif //ARBITER_SATNET_H
