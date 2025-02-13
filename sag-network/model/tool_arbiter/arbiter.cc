#include "ns3/arbiter.h"

namespace ns3 {

// Arbiter result

ArbiterResult::ArbiterResult(bool failed, uint32_t out_if_idx) {
    m_failed = failed;
    m_out_if_idx = out_if_idx;
}

bool ArbiterResult::Failed() {
    return m_failed;
}

uint32_t ArbiterResult::GetOutIfIdx() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve out interface index if the arbiter did not succeed in finding a next hop");
    }
    return m_out_if_idx;
}

ArbiterResultIPv4::ArbiterResultIPv4(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address)
:ArbiterResult(failed, out_if_idx) {
    m_gateway_ip_address = gateway_ip_address;
}

uint32_t ArbiterResultIPv4::GetGatewayIpAddress() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve gateway IP address if the arbiter did not succeed in finding a next hop");
    }
    return m_gateway_ip_address;
}

ArbiterResultIPv6::ArbiterResultIPv6(bool failed, uint32_t out_if_idx, uint8_t gateway_ipv6_address[16])
:ArbiterResult(failed, out_if_idx) {
    memcpy(m_gateway_ipv6_address, gateway_ipv6_address, 16);
}

void ArbiterResultIPv6::GetGatewayIpv6Address(uint8_t ipv6Address[16]) {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve gateway IP address if the arbiter did not succeed in finding a next hop");
    }
    memcpy(ipv6Address, m_gateway_ipv6_address, 16);
    return;
}



// Arbiter

NS_OBJECT_ENSURE_REGISTERED (Arbiter);
TypeId Arbiter::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Arbiter")
            .SetParent<Object> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

Arbiter::Arbiter()
{
    
}






}
