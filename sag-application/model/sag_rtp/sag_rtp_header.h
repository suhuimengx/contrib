/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Author: Xin Zhang <zanxin@smail.nju.edu.cn>
 *
 */

#ifndef SAG_RTP_HEADER_H
#define SAG_RTP_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"
#include <map>
#include <set>

namespace ns3 {

void RtpHdrSetBit (uint8_t& val, uint8_t pos, bool bit);
bool RtpHdrGetBit (uint8_t val, uint8_t pos);

const uint8_t RTP_VERSION = 2;

//-------------------- RTP HEADER (RFC 3550) ----------------------//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           timestamp                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |           synchronization source (SSRC) identifier            |
//  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  |            contributing source (CSRC) identifiers             |
//  |                             ....                              |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class SagRtpHeader : public Header
{
public:
  static TypeId GetTypeId (void);

  SagRtpHeader  ();
  SagRtpHeader (uint8_t payloadType);
  void SetPadding (bool padding);
  void SetMarker (bool marker);
  void SetExtension (bool extension);
  void SetPayloadType (uint8_t payloadType);
  void SetSequence (uint16_t sequence);
  void SetSsrc (uint32_t ssrc);
  void SetTimestamp (uint32_t timestamp);

  bool IsExtension () const;
  bool IsMarker () const;
  uint8_t GetPayloadType () const;
  uint16_t GetSequence () const;
  uint32_t GetSsrc () const;
  uint32_t GetTimestamp () const;
  const std::set<uint32_t>& GetCsrcs () const;
  uint8_t AddCsrc (uint32_t csrc);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  /**
     * \brief write the results of the SAG application to the base type
     *
     * \param start start of the iterator
     */
  virtual void Serialize (Buffer::Iterator start) const;
  /**
     * \brief write the results of the SAG application to the base type
     *
     * \param start start of the iterator
     */
  virtual uint32_t Deserialize (Buffer::Iterator start);


protected:
 //!< Identifier of what these sequences belong to
    bool m_padding;
    bool m_extension;
    bool m_marker;
    uint8_t m_payloadType;
    uint16_t m_sequence;
    uint32_t m_timestamp;
    uint32_t m_ssrc;
    std::set<uint32_t> m_csrcs;

};

//----------------- Common RCTP HEADER (RFC 3550) -----------------//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| Type/Cnt|       PT      |          length               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                 SSRC of RTCP packet sender                    |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class SagRtcpHeader : public Header
{
public:
    enum PayloadType {
        RTCP_SMPTETC = 194,
        RTCP_IJ      = 195,
        RTCP_SR      = 200,
        RTCP_RR      = 201,
        RTCP_SDES    = 202,
        RTCP_BYE     = 203,
        RTCP_APP     = 204,
        RTP_FB       = 205,
        RTP_PSFB     = 206,
        RTP_XR       = 207,
        RTP_RSI      = 209,
        RTP_TOKEN    = 210,
        RTP_IDMS     = 211,
        RTP_RSNM     = 213,
    };

    enum SdesType{
        RTCP_SDES_END   = 0,
        RTCP_SDES_CNAME = 1,
        RTCP_SDES_NAME  = 2,
        RTCP_SDES_EMAIL = 3,
        RTCP_SDES_PHONE = 4,
        RTCP_SDES_LOC   = 5,
        RTCP_SDES_TOOL  = 6,
        RTCP_SDES_NOTE  = 7,
        RTCP_SDES_PRIV  = 8,
        RTCP_SDES_APSI  = 10,
    };

    enum RtpFeedbackType {
        RTCP_RTPFB_GNACK  =  1,
        RTCP_RTPFB_TMMBR  =  3,
        RTCP_RTPFB_TMMBN  =  4,
        RTCP_RTPFB_SR_REQ =  5,
        RTCP_RTPFB_RAMS   =  6,
        RTCP_RTPFB_TLLEI  =  7,
        RTCP_RTPFB_ECN_FB =  8,
        RTCP_RTPFB_PR     =  9,
        RTCP_RTPFB_CC     = 15,  // TODO (deferred): Change to IANA-assigned value
    };

    SagRtcpHeader ();
    SagRtcpHeader (uint8_t packetType);
    SagRtcpHeader (uint8_t packetType, uint8_t subType);
    virtual ~SagRtcpHeader ();
    virtual void Clear ();

    static ns3::TypeId GetTypeId ();
    virtual ns3::TypeId GetInstanceTypeId () const;
    virtual uint32_t GetSerializedSize () const;
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    bool IsPadding () const;
    void SetPadding (bool padding);
    uint8_t GetTypeOrCount () const;
    void SetTypeOrCount (uint8_t typeOrCnt);
    uint8_t GetPacketType () const;
    void SetPacketType (uint8_t packetType);
    uint32_t GetSendSsrc () const;
    void SetSendSsrc (uint32_t sendSsrc);

protected:
    void PrintN (std::ostream& os) const;
    /**
       * \brief writes the header data to a buffer starting at the specified iterator position
       *
       * \param start start of the buffer
       */
    void SerializeCommon (ns3::Buffer::Iterator& start) const;
    /**
       * \brief reads the header data from a buffer starting at the specified iterator position
       *
       * \param start start of the buffer
       */
    uint32_t DeserializeCommon (ns3::Buffer::Iterator& start);

    bool m_padding;
    uint8_t m_typeOrCnt;
    uint8_t m_packetType;
    uint16_t m_length;
    uint32_t m_sendSsrc;
};

//-- RCTP CCFB HEADER (draft-ietf-avtcore-cc-feedback-message-01) -//
//   0                   1                   2                   3
//   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| FMT=CCFB| PT=RTPFB=205  |          length               |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                 SSRC of RTCP packet sender                    |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                   SSRC of 1st RTP Stream                      |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |          begin_seq            |             end_seq           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |L|ECN|  Arrival time offset    | ...                           .
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  .                                                               .
//  .                                                               .
//  .                                                               .
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                   SSRC of nth RTP Stream                      |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |          begin_seq            |             end_seq           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |L|ECN|  Arrival time offset    | ...                           |
//  .                                                               .
//  .                                                               .
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                        Report Timestamp                       |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
class CCFeedbackHeader : public SagRtcpHeader
{
public:
    class MetricBlock
    {
    public:
        static constexpr uint16_t m_overrange = 0x1FFE;
        static constexpr uint16_t m_unavailable = 0x1FFF;
        uint8_t m_ecn;
        uint64_t m_timestampUs;
        uint16_t m_ato;
    };

    enum RejectReason {
        CCFB_NONE,      /**< Feedback was added correctly */
        CCFB_DUPLICATE, /**< Feedback of duplicate packet */
        CCFB_BAD_ECN,   /**< ECN value takes more than two bits */
        CCFB_TOO_LONG,  /**< Adding this sequence number would make the packet too long */
    };
    typedef std::map<uint64_t /* sequence */, MetricBlock> ReportBlock_t;

    CCFeedbackHeader ();
    virtual ~CCFeedbackHeader ();
    virtual void Clear ();

    static ns3::TypeId GetTypeId ();
    virtual ns3::TypeId GetInstanceTypeId () const;
    virtual uint32_t GetSerializedSize () const;
    /**
       * \brief writes the header data to a buffer starting at the specified iterator position
       *
       * \param start of the buffer
       */
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    /**
       * \brief reads the header data from a buffer starting at the specified iterator position
       *
       * \param start of the buffer
       */
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    RejectReason AddFeedback (uint32_t ssrc, uint16_t seq, uint64_t timestampUs, uint8_t ecn=0);
    uint8_t Empty () const;
    void GetSsrcList (std::set<uint32_t>& rv) const;
    uint8_t GetMetricList (uint32_t ssrc, std::vector<std::pair<uint16_t, MetricBlock> >& rv) const;

protected:
    static std::pair<uint16_t, uint16_t> CalculateBeginStopSeq (const ReportBlock_t& rb);
    static uint64_t NtpToUs (uint32_t ntp);
    static uint32_t UsToNtp (uint64_t tsUs);
    static uint16_t NtpToAto (uint32_t ntp, uint32_t ntpRef);
    static uint32_t AtoToNtp (uint16_t ato, uint32_t ntpRef);

    uint8_t UpdateLength ();
    std::map<uint32_t /* SSRC */, ReportBlock_t> m_reportBlocks;
    uint64_t m_latestTsUs;
};

} // namespace ns3

#endif /* SAG_RTP_HEADER_H */
