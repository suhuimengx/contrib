//
// Created by Mengy on 2023/1/30.
//

#ifndef SAG_PLATFORM_MASTER_IADR_LINK_STATE_PACKET_H
#define SAG_PLATFORM_MASTER_IADR_LINK_STATE_PACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include <utility>


#include "ns3/nstime.h"

#include "ns3/iadr-lsa-identifier.h"


namespace ns3 {
namespace iadr {

struct IADRLinkStateIdentifier;
class TypeHeader;
class LSAHeader;
class LSALinkData;
//class LSAStreamData;
//class LSRHeader;
//class LSRPacket;


/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

/**
* \ingroup iadr
* \brief   LSA Packet Header Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |            LS age             |     Options   |   LSA type    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                        Link State ID                          |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     Advertising Router                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     LS sequence number                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         LS checksum           |             length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  \endverbatim
*/

class LSAHeader : public Header {
public:
    /**
     * constructor
     *
     * \param
     */
    LSAHeader(uint32_t LSage = 0, uint8_t options = 0, Ipv4Address LinkStateID = Ipv4Address(),
              Ipv4Address AdvertisingRouter = Ipv4Address(), uint32_t LSsequence = 0,
              uint8_t checkSum = 0, uint8_t PacketLength = 0, uint32_t addedTime = 0);

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

    bool operator==(LSAHeader const &o) const;

/*    OSPFLinkStateIdentifier GetIdentifier() {
    	OSPFLinkStateIdentifier identi(m_LSAtype,m_LinkStateID,m_AdvertisingRouter);
        return identi;
    }*/

    uint32_t GetLSAddedTime() {
		return m_addTime;
	}

    void SetLSAddedTime(uint32_t s) {
		m_addTime = s;
	}


    uint32_t GetLSAge() {
        return m_LSage;
    }

    void SetLSAge(uint32_t LSage){
        m_LSage = LSage;
    }

    void SetLSSequence(uint32_t LSsequence){
    	m_LSsequence = LSsequence;
	}

    uint32_t GetLSSequence() {
        return m_LSsequence;
    }

    Ipv4Address GetLinkStateID() {
        return m_LinkStateID;
    }

    Ipv4Address GetAdvertisiongRouter() {
        return m_AdvertisingRouter;
    }

    uint8_t GetPacketLength() {
        return m_PacketLength;
    }
    uint8_t GetLSAType()
    {
    	return m_LSAtype;
    }
    uint8_t GetCheckSum()
    {
    	return m_checksum;
    }

    uint8_t GetOption()
    {
    	return m_options;
    }



private:
    uint32_t m_LSage;          ///< LSA Age
    uint8_t m_options;       ///< options
    uint8_t m_LSAtype;  ///< type = 1(router-lsa)
    Ipv4Address m_LinkStateID;      ///< for router-lsa, LinkStateID = The Originated Router's ID
    Ipv4Address m_AdvertisingRouter;    ///< OriginatedRouterID
    uint32_t m_LSsequence;        ///< LSA Sequence
    uint8_t m_checksum;          ///<<checksum
    uint8_t m_PacketLength;      ///< packet length including LSA header

    uint32_t m_addTime;    /// unit(seconds)


};



/**
* \ingroup iadr
* \brief   LSA Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    0    |U|D|A|       0       |           # streams           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    0    |V|E|B|       0       |           # links             |

       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Source Router                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Destination Router                    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      Type     |     # QoS     |           datamount           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      QoS      |      0        |          QoS metric           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Source Router                       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Destination Router                    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              ...                              |


       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Link ID                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Link Data                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      Type     |     # TOS     |           metric              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      TOS      |      0        |          TOS metric           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          Link ID                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Link Data                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              ...                              |

  \endverbatim
*/
class LSAPacket : public Header {
public:
    /**
     * constructor
     *
     * \param
     */
   //LSAPacket(uint8_t UDA = 0, uint8_t streamn = 0, std::vector<LSAStreamData> LSAStreamDatas = {});
    LSAPacket(uint8_t VEB = 0, uint8_t linkn = 0, std::vector<LSALinkData> LSALinkDatas = {});

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

    bool operator==(LSAPacket const &o) const;


    /*uint8_t GetUDA() {
        return m_UDA;
    }

    uint8_t Getstreamn() {
        return m_streamn;
    }

    std::vector<LSAStreamData> GetLSAStreamDatas() {
        return m_LSAStreamDatas;
    }*/

    uint8_t GetVEB() {
        return m_VEB;
    }

    uint8_t Getlinkn() {
        return m_linkn;
    }

    std::vector<LSALinkData> GetLSALinkDatas() {
        return m_LSALinkDatas;
    }


private:
    /*uint8_t m_UDA;          ///< U\D\A = 0\0\0
    uint8_t m_streamn;       ///< stream number
    std::vector<LSAStreamData> m_LSAStreamDatas = {}; ///< LSA Stream Data*/

    uint8_t m_VEB;          ///< V\E\B = 0\0\0
    uint8_t m_linkn;       ///< link number
    std::vector<LSALinkData> m_LSALinkDatas = {}; ///< LSA Link Data

};


class LSALinkData : public Header {
public:
    /**
     * constructor
     *
     * \param
     */

    LSALinkData(Ipv4Address linkID = Ipv4Address(), Ipv4Address linkdata = Ipv4Address(), Ipv4Address neighborlinkID = Ipv4Address(), uint8_t metric = 0, uint16_t metric2 = 0, uint8_t metric3 = 0);

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


    Ipv4Address GetlinkID() {
        return m_linkID;
    }

    Ipv4Address Getlinkdata() {
        return m_linkdata;
    }

    Ipv4Address GetneighborlinkID() {
	    return m_neighborlinkID;
    }

    uint8_t GetLinkType() {
        return m_linktype;
    }

    uint8_t GetToSn() {
        return m_TOSn;
    }

    uint8_t Getmetric() {
        return m_metric;
    }
    uint16_t Getmetric2() {
        return m_metric2;
    }
    uint8_t Getmetric3() {
        return m_metric3;
    }

    uint8_t GetTOS() {
        return m_TOS;
    }

    uint8_t GetTOSmetric() {
        return m_TOSmetric;
    }


    bool operator==(LSALinkData const &o) const;


private:
    Ipv4Address m_linkID; ///< link ID: neighbor's router ID
    Ipv4Address m_linkdata; /// < link data: my interface ip address


    Ipv4Address m_neighborlinkID; ///< neighbor link ID: neighbor's interface


    uint8_t m_linktype;          ///< link type = 1
    uint8_t m_TOSn;        ///< TOS number
    uint8_t m_metric;        /// link metric
    uint16_t m_metric2;        /// link metric
    uint8_t m_metric3;        /// link metric
    uint8_t m_TOS;       ///< TOS
    uint8_t m_TOSmetric;  /// TOS metric

};



/*class LSAStreamData : public Header {
public:
    *
     * constructor
     *
     * \param


    LSAStreamData(Ipv4Address SourceRouter = Ipv4Address(), Ipv4Address DestinationRouter = Ipv4Address(), uint8_t datamount = 0);

    *
     * \brief Get the type ID.
     * \return the object TypeId

    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;


    Ipv4Address GetSourceRouter() {
        return m_SourceRouter;
    }

    Ipv4Address GetDestinationRouter() {
        return m_DestinationRouter;
    }

    uint8_t GetStreamType() {
        return m_streamtype;
    }

    uint8_t GetQoSn() {
        return m_QoSn;
    }

    uint8_t Getdatamount() {
        return m_datamount;
    }

    uint8_t GetQoS() {
        return m_QoS;
    }

    uint8_t GetQoSmetric() {
        return m_QoSmetric;
    }


    bool operator==(LSAStreamData const &o) const;


private:
    Ipv4Address m_SourceRouter; ///< link ID: neighbor's router ID
    Ipv4Address m_DestinationRouter; /// < link data: my interface ip address
    uint8_t m_streamtype;          ///< link type = 1
    uint8_t m_QoSn;        ///< TOS number
    uint8_t m_datamount;        /// link metric
    uint8_t m_QoS;       ///< TOS
    uint8_t m_QoSmetric;  /// TOS metric
};*/





/**
* \ingroup ospf
* \brief   LSU Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                           # LSAs                              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                            LSA                                |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             ...                               |

  \endverbatim
*/



class LSUHeader : public Header {
public:
     /**
     * constructor
     *
     * \param
     */

    LSUHeader(std::vector<std::pair<LSAHeader,LSAPacket>> LSAs = {});

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

    bool operator==(LSUHeader const &o) const;

    uint32_t GetlSAnumber() {
        return m_LSAn;
    }

    std::vector<std::pair<LSAHeader,LSAPacket>> GetLSAs() {
        return m_LSAs;
    }


private:
    uint32_t m_LSAn;          ///< LSA number
    std::vector<std::pair<LSAHeader,LSAPacket>> m_LSAs;//< LSAs

};



/**
* \ingroup ospf
* \brief   LSR Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                        Link State Type                        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         Link State ID                         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                       Advertising Router                      |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                 ...                           |

  \endverbatim
  *//*


class LSRHeader : public Header {
public:
    */
/**
     * constructor
     *
     * \param
     *//*


    LSRHeader(std::vector<LSRPacket> LSRs = {});

    */
/**
     * \brief Get the type ID.
     * \return the object TypeId
     *//*


    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;

    bool operator==(LSRHeader const &o) const;

    std::vector<LSRPacket> GetLSRs() {
        return m_LSRs;
    }


private:
    std::vector<LSRPacket> m_LSRs;       ///< LSRs
    uint32_t m_LSRPacketNum;

};

class LSRPacket : public Header {
public:
    */
/**
     * constructor
     *
     * \param
     *//*


    LSRPacket(Ipv4Address LinkStateID = Ipv4Address(), Ipv4Address AdvertisingRouter = Ipv4Address());

    */
/**
     * \brief Get the type ID.
     * \return the object TypeId
     *//*


    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;

    bool operator==(LSRPacket const &o) const;

    uint32_t GetLSType() {
        return m_LinkStateType;
    }

    Ipv4Address GetLSID() {
        return m_LinkStateID;
    }
    Ipv4Address GetAdRouter() {
            return m_AdvertisingRouter;
    }


private:
    uint32_t m_LinkStateType;          ///< for router-lsa. linkstatetype = 1
    Ipv4Address m_LinkStateID;         ///< for router-lsa, linkstateID = Originated Router ID
    Ipv4Address m_AdvertisingRouter;    ///< OriginatedRouterID

};

*/
/**
* \ingroup ospf
* \brief   LSA Ack Packet Format
  \verbatim
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                           LSA Header                          |
        |                                                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                             ...                               |

  \endverbatim
 *//*


class LSAackHeader : public Header {
public:
    */
/**
     * constructor
     *
     * \param
     *//*


    LSAackHeader(std::vector<LSAHeader> LSAacks = {});

    */
/**
     * \brief Get the type ID.
     * \return the object TypeId
     *//*


    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const;

    uint32_t GetSerializedSize() const;

    void Serialize(Buffer::Iterator start) const;

    uint32_t Deserialize(Buffer::Iterator start);

    void Print(std::ostream &os) const;

    bool operator==(LSAackHeader const &o) const;

    std::vector<LSAHeader> GetLSAacks() {
        return m_LSAacks;
    }


private:
    std::vector<LSAHeader> m_LSAacks;       ///< LSAacks
    uint32_t m_ackNum;

};*/

}  // namespace iadr
}  // namespace ns3





#endif //SAG_PLATFORM_MASTER_IADR_LINK_STATE_PACKET_H
