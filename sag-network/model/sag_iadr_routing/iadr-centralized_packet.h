//
// Created by Yilei Wang on 2023/2/21.
//

#ifndef CENTRALIZED_PACKET_H
#define CENTRALIZED_PACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include <utility>
//#include "iadr_routing_table.h"


#include "ns3/nstime.h"

//#include "cr_lsa_identifier.h"


namespace ns3 {
namespace iadr {


class TypeHeader;
class RTSHeader;




/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

/**
* \ingroup dr
* \brief   Centralized Packet Header Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |            Path age             |     length                  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         source Router ID                      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          source Area ID                       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         dest Router ID                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          dest Area ID                         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  \endverbatim
*/

class RTSHeader : public Header {
public:
    /**
     * constructor
     *
     * \param
     */
    //RTSHeader(uint16_t Pathage = 0, uint16_t PathLength = 0, Ipv4Address src_routerID = Ipv4Address (), Ipv4Address src_areaID = Ipv4Address (), Ipv4Address dst_routerID = Ipv4Address (), Ipv4Address dst_areaID = Ipv4Address ());

	RTSHeader(uint16_t Pathage = 0, uint16_t PathLength = 0, Ipv4Address local_routerID = Ipv4Address (), Ipv4Address local_areaID = Ipv4Address ());

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;

    bool operator==(RTSHeader const &o) const;

/*    OSPFLinkStateIdentifier GetIdentifier() {
    	OSPFLinkStateIdentifier identi(m_LSAtype,m_LinkStateID,m_AdvertisingRouter);
        return identi;
    }*/

    uint16_t GetPathage() const {
        return m_Pathage;
    }

    void SetPathage(uint16_t Pathage)  const {
        m_Pathage = Pathage;
    }

    uint16_t GetPathLength()  const {
        return m_PathLength;
    }

    void SetPathLength(uint16_t PathLength)  const {
            m_PathLength = PathLength;
     }

/*
    Ipv4Address GetSrc_RouterID ()  const {
        return m_src_routerID;
    }

    Ipv4Address GetSrc_AreaID ()  const {
        return m_src_areaID;
    }

    Ipv4Address GetDst_RouterID ()  const {
        return m_dst_routerID;
    }

    Ipv4Address GetDst_AreaID ()  const {
        return m_dst_areaID;
    }

*/
    Ipv4Address GetLocal_AreaID ()  const {
        return m_local_areaID;
    }

    Ipv4Address GetLocal_RouterID ()  const {
        return m_local_routerID;
    }
    //std::vector<std::pair<RTSHeader,RTSPacket>> GetRTSs() {
        //return m_RTSs;
    //}

   private:
    mutable uint16_t m_Pathage;          ///< Path Age
    mutable uint16_t m_PathLength;      ///< path length
    /*mutable Ipv4Address m_src_routerID;
    mutable Ipv4Address m_src_areaID;
    mutable Ipv4Address m_dst_routerID;
    mutable Ipv4Address m_dst_areaID;
    */
    mutable Ipv4Address m_local_routerID;
    mutable Ipv4Address m_local_areaID;
    //std::vector<std::pair<RTSHeader,RTSPacket>> m_RTSs;//< LSAs


};

/**
* \ingroup DR
* \brief   CR Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    Path Router:                                                |
       |                       Router ID  1                            |
       |                       Router ID  2                            |
       |                              ...                              |
       |    Path Area:                                                |
       |                         Area ID  1                            |
       |                         Area ID  2                            |
       |                              ...                              |

  \endverbatim
*/
class RTSPacket : public Header {
public:
    /**
     * constructor
     *
     * \param
     */
    //RTSPacket(std::vector<Ipv4Address> RTSPathRouters = {},std::vector<Ipv4Address> RTSPathAreas = {});
	RTSPacket(std::vector<Ipv4Address> srcip = {},std::vector<Ipv4Address> dst = {},std::vector<Ipv4Address> nexthop = {});
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;

    bool operator==(RTSPacket const &o) const;

/*
    std::vector<Ipv4Address> GetRTSPathRouters()  const {
        return m_RTSPathRouters;
    }
    std::vector<Ipv4Address> GetRTSPathAreas()  const {
        return m_RTSPathAreas;
    }
*/
    std::vector<Ipv4Address> Getsrcip()  const {
        return m_srcip;
    }
    std::vector<Ipv4Address> Getdst()  const {
        return m_dst;
    }
    std::vector<Ipv4Address> Getnexthop()  const {
        return m_nexthop;
    }
    uint32_t GetSize() const {
    	return m_size;
    }

private:
    //mutable std::vector<Ipv4Address> m_RTSPathRouters = {}; ///< Path Data
    //mutable std::vector<Ipv4Address> m_RTSPathAreas = {}; ///< Path Data
    std::vector<Ipv4Address> m_srcip;///
    std::vector<Ipv4Address> m_dst;
    std::vector<Ipv4Address> m_nexthop;
    uint32_t m_size;

};

/**
* \ingroup CR
* \brief   RTSP Header Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                        RTS Header                             |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         RTS Packet                            |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                 ...                           |

  \endverbatim
  */

        class RTSPHeader : public Header {
        public:
            /**
             * constructor
             *
             * \param
             */

            RTSPHeader(std::pair<RTSHeader, RTSPacket> RTSs = {});

            /**
             * \brief Get the type ID.
             * \return the object TypeId
             */

            static TypeId GetTypeId();

            TypeId GetInstanceTypeId() const;

            uint32_t GetSerializedSize() const;

            void Serialize(Buffer::Iterator start) const;

            uint32_t Deserialize(Buffer::Iterator start);

            void Print(std::ostream &os) const;

            bool operator==(RTSPHeader const &o) const;

            std::pair<RTSHeader, RTSPacket> GetRTSs() {
                return m_RTSs;
            }



        private:
            std::pair<RTSHeader, RTSPacket> m_RTSs;       ///< LSRs

        };


}  // namespace dr
}  // namespace ns3




#endif //CENTRALIZED_PACKET_H
