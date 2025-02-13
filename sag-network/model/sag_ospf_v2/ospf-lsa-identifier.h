/*
 * Copyright (c) 2023 NJU
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
 * Author: Xiaoyu Liu <xyliu0119@163.com>
 */

#ifndef OSPF_LS_IDENTIFIER_H
#define OSPF_LS_IDENTIFIER_H

#include <set>
#include <iostream>


#include "ns3/ipv4-address.h"

using namespace ns3;

namespace ns3 {
namespace ospf {
struct OSPFLinkStateIdentifier {
    // LSA Identifier
    uint8_t m_LSAtype;
    Ipv4Address m_LinkStateID;
    Ipv4Address m_AdvertisingRouter;

    // LSA Identifier Constructor
    OSPFLinkStateIdentifier() {}
    OSPFLinkStateIdentifier(uint8_t LSAtype, Ipv4Address m_LinkStateID, Ipv4Address AdvertisingRouter)
        : m_LSAtype(LSAtype),m_LinkStateID(m_LinkStateID), m_AdvertisingRouter(AdvertisingRouter) {}

    //Whether this LSA is originated by Router_ID
    bool IsOriginatedBy (Ipv4Address router_ID) const {
        return m_AdvertisingRouter == router_ID;
    }

    //Change Type
    OSPFLinkStateIdentifier As(uint8_t type) const {
        return OSPFLinkStateIdentifier(type, m_LinkStateID, m_AdvertisingRouter);
    }

    // Whether equal
    bool operator== (const OSPFLinkStateIdentifier &other) const {
        return (
                m_LSAtype == other.m_LSAtype &&
                m_LinkStateID == other.m_LinkStateID &&
                m_AdvertisingRouter == other.m_AdvertisingRouter
        );
    }

    // Whether <
    bool operator< (const OSPFLinkStateIdentifier &other) const {
        return (
                m_LSAtype < other.m_LSAtype ||
            (m_LSAtype == other.m_LSAtype && m_LinkStateID < other.m_LinkStateID) ||
            (m_LSAtype == other. m_LSAtype && m_LinkStateID == other.m_LinkStateID && m_AdvertisingRouter < other.m_AdvertisingRouter)
        );
    }


};
}
}

/*std::ostream& operator<< (std::ostream& os, const OSPFLinkStateIdentifier& id);
}
}*/

#endif
