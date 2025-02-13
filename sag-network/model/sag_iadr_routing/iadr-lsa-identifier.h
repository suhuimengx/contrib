#ifndef IADR_LS_IDENTIFIER_H
#define IADR_LS_IDENTIFIER_H

#include <set>
#include <iostream>


#include "ns3/ipv4-address.h"

using namespace ns3;

namespace ns3 {
namespace iadr {
struct IADRLinkStateIdentifier {
    // LSA Identifier
    uint8_t m_LSAtype;
    Ipv4Address m_LinkStateID;
    Ipv4Address m_AdvertisingRouter;

    // LSA Identifier Constructor
    IADRLinkStateIdentifier() {}
    IADRLinkStateIdentifier(uint8_t LSAtype, Ipv4Address m_LinkStateID, Ipv4Address AdvertisingRouter)
        : m_LSAtype(LSAtype),m_LinkStateID(m_LinkStateID), m_AdvertisingRouter(AdvertisingRouter) {}

    //Whether this LSA is originated by Router_ID
    bool IsOriginatedBy (Ipv4Address router_ID) const {
        return m_AdvertisingRouter == router_ID;
    }

    //Change Type
    IADRLinkStateIdentifier As(uint8_t type) const {
        return IADRLinkStateIdentifier(type, m_LinkStateID, m_AdvertisingRouter);
    }

    // Whether equal
    bool operator== (const IADRLinkStateIdentifier &other) const {
        return (
                m_LSAtype == other.m_LSAtype &&
                m_LinkStateID == other.m_LinkStateID &&
                m_AdvertisingRouter == other.m_AdvertisingRouter
        );
    }

    // Whether <
    bool operator< (const IADRLinkStateIdentifier &other) const {
        return (
                m_LSAtype < other.m_LSAtype ||
            (m_LSAtype == other.m_LSAtype && m_LinkStateID < other.m_LinkStateID) ||
            (m_LSAtype == other. m_LSAtype && m_LinkStateID == other.m_LinkStateID && m_AdvertisingRouter < other.m_AdvertisingRouter)
        );
    }


};
}
}

/*std::ostream& operator<< (std::ostream& os, const IADRLinkStateIdentifier& id);
}
}*/

#endif
