#ifndef ARBITER_H
#define ARBITER_H


#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include "ns3/topology.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-header.h"
#include "ns3/exp-util.h"

namespace ns3 {

class ArbiterResult {

public:
    ArbiterResult(bool failed, uint32_t out_if_idx);
    bool Failed();
    uint32_t GetOutIfIdx();
    // virtual uint32_t GetGatewayIpAddress()=0;

protected:
    bool m_failed;
    uint32_t m_out_if_idx;

};

class ArbiterResultIPv4 : public ArbiterResult{
public:
    ArbiterResultIPv4(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address);
    uint32_t GetGatewayIpAddress();
private:
    uint32_t m_gateway_ip_address;
};

class ArbiterResultIPv6 : public ArbiterResult{
public:
    ArbiterResultIPv6(bool failed, uint32_t out_if_idx, uint8_t gateway_ipv6_address[16]);
    void GetGatewayIpv6Address(uint8_t ipv6Address[16]);
private:
    uint8_t m_gateway_ipv6_address[16];
};

class Arbiter : public Object
{
// Basic class of routing table

public:
    static TypeId GetTypeId (void);
    Arbiter();

    //Arbiter(Ptr<Node> this_node, NodeContainer nodes);
    virtual void DoInitialize(Ptr<Node> this_node, NodeContainer nodes)=0;

    /**
     * Base decide how to forward. Directly called by sag-routing-protocal.
     * It does some nice pre-processing and checking and calls Decide() of the
     * subclass to actually make the decision. Virtual here to select ip address
     * or nodeid to maintain your routing table.
     * 
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     *
     * @return Routing arbiter result.
     */
    virtual ArbiterResultIPv4 BaseDecide(
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader
    ) = 0;

    /**
     * Decide what should be done with the result.
     *
     * @param source_node_id                    Node where the packet originated from
     * @param target_node_id                    Node where the packet has to go to
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     * @param is_socket_request_for_source_ip   True iff there is not actually forwarding being done, but it is only
     *                                          a dummy packet sent by the socket to decide the source IP address.
     *                                          Most importantly, this means that THERE IS NO OTHER HEADER IN THE
     *                                          PACKET, IT IS EMPTY (EVEN IF THE PROTOCOL FIELD IS SET IN THE IP
     *                                          HEADER).
     *
     * @return Routing arbiter result.
     *
     *         If it is a socket request for source IP AND you signal it failed, it is not a drop but it will send
     *         a direct signal to the socket that there is no route, likely terminating the socket directly.
     *         In other cases, when you set it failed, it leads to a drop.
     *
     *         A TCP socket first asks for source IP, and then subsequently the tcp-l4 layer does another
     *         call with the full header to get the real decision.
     *
     *         A UDP source only asks for source IP, and does not do another subsequent call in the udp-l4 layer.
     */
    virtual ArbiterResultIPv4 Decide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    ) = 0;

    /**
     * Base decide how to forward. Directly called by sag-routing-protocal.
     * It does some nice pre-processing and checking and calls Decide() of the
     * subclass to actually make the decision. Virtual here to select ip address
     * or nodeid to maintain your routing table.
     * 
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     *
     * @return Routing arbiter result.
     */
    virtual ArbiterResultIPv6 BaseDecide(
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv6Header const &ipHeader
    ) = 0;

    /**
     * Decide what should be done with the result.
     *
     * @param source_node_id                    Node where the packet originated from
     * @param target_node_id                    Node where the packet has to go to
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     * @param is_socket_request_for_source_ip   True iff there is not actually forwarding being done, but it is only
     *                                          a dummy packet sent by the socket to decide the source IP address.
     *                                          Most importantly, this means that THERE IS NO OTHER HEADER IN THE
     *                                          PACKET, IT IS EMPTY (EVEN IF THE PROTOCOL FIELD IS SET IN THE IP
     *                                          HEADER).
     *
     * @return Routing arbiter result.
     *
     *         If it is a socket request for source IP AND you signal it failed, it is not a drop but it will send
     *         a direct signal to the socket that there is no route, likely terminating the socket directly.
     *         In other cases, when you set it failed, it leads to a drop.
     *
     *         A TCP socket first asks for source IP, and then subsequently the tcp-l4 layer does another
     *         call with the full header to get the real decision.
     *
     *         A UDP source only asks for source IP, and does not do another subsequent call in the udp-l4 layer.
     */
    virtual ArbiterResultIPv6 Decide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv6Header const &ipHeader,
            bool is_socket_request_for_source_ip
    ) = 0;

    /**
     * Convert the forwarding state (i.e., routing table) to a string representation.
     *
     * @return String representation
     */
    virtual std::string StringReprOfForwardingState() = 0;

    // virtual void SetIpAddresstoNodeId(std::map<uint32_t, uint32_t>& Ip2NodeId) = 0;

protected:


private:


};

}

#endif //ARBITER_H
