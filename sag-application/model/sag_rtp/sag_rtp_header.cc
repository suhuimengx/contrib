/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Author: Xin Zhang <zanxin@smail.nju.edu.cn>
 *
 */

#include "sag_rtp_header.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"

#include <set>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SagRtpHeader");

NS_OBJECT_ENSURE_REGISTERED (SagRtpHeader);
NS_OBJECT_ENSURE_REGISTERED (SagRtcpHeader);
NS_OBJECT_ENSURE_REGISTERED (CCFeedbackHeader);

void RtpHdrSetBit (uint8_t& val, uint8_t pos, bool bit)
{
    NS_ASSERT (pos < 8);
    if (bit) {
        val |= (1u << pos);
    } else {
        val &= (~(1u << pos));
    }
}

bool RtpHdrGetBit (uint8_t val, uint8_t pos)
{
    return bool (val & (1u << pos));
}

TypeId
SagRtpHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SagRtpHeader")
            .SetParent<Header> ()
            .SetGroupName("Applications")
            .AddConstructor<SagRtpHeader> ()
    ;
    return tid;
}

SagRtpHeader::SagRtpHeader  ()
: Header{}
, m_padding{false}
, m_extension{false}
, m_marker{false}
, m_payloadType{0}
, m_sequence{0}
, m_timestamp{0}
, m_ssrc{0}
, m_csrcs{}
{
  NS_LOG_FUNCTION (this);
}

SagRtpHeader::SagRtpHeader (uint8_t payloadType)
: Header{}
, m_padding{false}
, m_extension{false}
, m_marker{false}
, m_payloadType{payloadType}
, m_sequence{0}
, m_timestamp{0}
, m_ssrc{0}
, m_csrcs{}
{}

TypeId
SagRtpHeader::GetInstanceTypeId (void) const
{
    NS_LOG_FUNCTION (this);
    return GetTypeId ();
}

void
SagRtpHeader::SetPadding (bool padding)
{
	  NS_LOG_FUNCTION (this<<padding);
	  m_padding=padding;
}

void
SagRtpHeader::SetMarker (bool marker)
{
	 NS_LOG_FUNCTION (this<<marker);
	 m_marker=marker;
}

void
SagRtpHeader::SetExtension (bool extension)
{
	 NS_LOG_FUNCTION (this<<extension);
	 m_extension=extension;
}

void
SagRtpHeader::SetPayloadType (uint8_t payloadType)
{
	 NS_LOG_FUNCTION (this<<payloadType);
	 m_payloadType=payloadType;
}

void
SagRtpHeader::SetSequence (uint16_t sequence)
{
	 NS_LOG_FUNCTION (this<<sequence);
	 m_sequence=sequence;
}

void
SagRtpHeader::SetSsrc (uint32_t ssrc)
{
	 NS_LOG_FUNCTION (this<<ssrc);
	 m_ssrc=ssrc;
}

void
SagRtpHeader::SetTimestamp (uint32_t timestamp)
{
	 NS_LOG_FUNCTION (this<<timestamp);
	 m_timestamp=timestamp;
}

bool
SagRtpHeader::IsExtension () const
{
	 NS_LOG_FUNCTION (this);
	 return m_extension;
}

bool
SagRtpHeader::IsMarker () const
{
	 NS_LOG_FUNCTION (this);
	 return m_marker;
}

uint8_t
SagRtpHeader::GetPayloadType () const
{
	 NS_LOG_FUNCTION (this);
	 return m_payloadType;
}

uint16_t
SagRtpHeader::GetSequence () const
{
	 NS_LOG_FUNCTION (this);
	 return m_sequence;
}

uint32_t
SagRtpHeader::GetSsrc () const
{
	 NS_LOG_FUNCTION (this);
	 return m_ssrc;
}

uint32_t
SagRtpHeader::GetTimestamp () const
{
	 NS_LOG_FUNCTION (this);
	 return m_timestamp;
}

const std::set<uint32_t>&
SagRtpHeader::GetCsrcs () const
{
	 NS_LOG_FUNCTION (this);
	 return m_csrcs;
}

uint8_t
SagRtpHeader::AddCsrc (uint32_t csrc)
{
	if (m_csrcs.count (csrc) != 0) {
	    return false;
	}
	m_csrcs.insert (csrc);
	return true;
}


uint32_t
SagRtpHeader::GetSerializedSize (void) const
{
	NS_LOG_FUNCTION (this);
	return 2+
			sizeof(m_sequence)+
			sizeof(m_timestamp)+
			sizeof(m_ssrc)+
			(m_csrcs.size()& 0x0f)*sizeof(decltype(m_csrcs)::value_type);
}

void
SagRtpHeader::Serialize (Buffer::Iterator start) const
{
    NS_ASSERT (m_csrcs.size () <= 0x0f);
    NS_ASSERT (m_payloadType <= 0x7f);

    const uint8_t csrcCount = (m_csrcs.size () & 0x0f);
    uint8_t octet1 = 0;
    octet1 |= (RTP_VERSION << 6);
    RtpHdrSetBit (octet1, 5, m_padding);
    RtpHdrSetBit (octet1, 4, m_extension);
    octet1 |= uint8_t (csrcCount);
    start.WriteU8 (octet1);

    uint8_t octet2 = 0;
    RtpHdrSetBit (octet2, 7, m_marker);
    octet2 |= (m_payloadType & 0x7f);
    start.WriteU8 (octet2);

    //start.WriteHtonU64 (m_id);
    start.WriteHtonU16 (m_sequence);
    start.WriteHtonU32 (m_timestamp);
    start.WriteHtonU32 (m_ssrc);
    for (const auto& csrc : m_csrcs) {
        start.WriteHtonU32 (csrc);
    }
}

uint32_t
SagRtpHeader::Deserialize (Buffer::Iterator start)
{
//  NS_LOG_FUNCTION (this << &start);
//  Buffer::Iterator i = start;
//  m_padding = i.ReadNtohU8 ();
//  m_extension = i.ReadNtohU8 ();
//  m_marker = i.ReadNtohU8 ();
//  m_payloadType = i.ReadNtohU8 ();
//  m_sequence = i.ReadNtohU16 ();
//  m_timestamp = i.ReadNtohU32 ();
//  m_ssrc = i.ReadNtohU32 ();
//  m_csrcs = i.ReadNtohU32 ();
//  return GetSerializedSize ();
    const auto octet1 = start.ReadU8 ();
    const uint8_t version = (octet1 >> 6);
    m_padding = RtpHdrGetBit (octet1, 5);
    m_extension = RtpHdrGetBit (octet1, 4);
    const uint8_t csrcCount = (octet1 & 0x0f);

    const auto octet2 = start.ReadU8 ();
    m_marker = RtpHdrGetBit (octet2, 7);
    m_payloadType = (octet2 & 0x7f);

    //m_id = start.ReadNtohU64 ();
    m_sequence = start.ReadNtohU16 ();
    m_timestamp = start.ReadNtohU32 ();
    m_ssrc = start.ReadNtohU32 ();
    m_csrcs.clear ();
    for (auto i = 0u; i < csrcCount; ++i) {
        const uint32_t csrc = start.ReadNtohU32 ();
        NS_ASSERT (m_csrcs.count (csrc) == 0);
        m_csrcs.insert (csrc);
    }
    NS_ASSERT (version == RTP_VERSION);
    return GetSerializedSize ();
}

void
SagRtpHeader::Print (std::ostream &os) const
{
//	NS_LOG_FUNCTION (this << &os);
//	os << "(padding=" << m_padding << ", extension=" << m_extension << ", marker=" << m_marker << ")";
//	os << "(payloadType=" << m_payloadType << ", sequence=" << m_sequence << ", timestamp=" << m_timestamp << ")";
//	os << "(ssrc=" << m_ssrc << ", csrcs =" << m_csrcs  << ")";
    NS_ASSERT (m_csrcs.size () <= 0x0f);
    os << "RtpHeader - version = " << int (RTP_VERSION)
       << ", padding = " << (m_padding ? "yes" : "no")
       << ", extension = " << (m_extension ? "yes" : "no")
       << ", CSRC count = " << m_csrcs.size ()
       << ", marker = " << (m_marker ? "yes" : "no")
       << ", payload type = " << int (m_payloadType)
       << ", sequence = " << m_sequence
       << ", timestamp = " << m_timestamp
       << ", ssrc = " << m_ssrc;
    size_t i = 0;
    for (const auto& csrc : m_csrcs) {
        os << ", CSRC#" << i << " = " << csrc;
        ++i;
    }
    os << std::endl;
}

SagRtcpHeader::SagRtcpHeader ()
: Header{}
, m_padding{false}
, m_typeOrCnt{0}
, m_packetType{0}
, m_length{1}
, m_sendSsrc{0}
{}

SagRtcpHeader::SagRtcpHeader (uint8_t packetType)
: Header{}
, m_padding{false}
, m_typeOrCnt{0}
, m_packetType{packetType}
, m_length{1}
, m_sendSsrc{0}
{}

SagRtcpHeader::SagRtcpHeader (uint8_t packetType, uint8_t subType)
: Header{}
, m_padding{false}
, m_typeOrCnt{subType}
, m_packetType{packetType}
, m_length{1}
, m_sendSsrc{0}
{
    NS_ASSERT (subType <= 0x1f);
}

SagRtcpHeader::~SagRtcpHeader () {}

void SagRtcpHeader::Clear ()
{
    m_padding = false;
    m_typeOrCnt = 0;
    m_packetType = 0;
    m_length = 1;
    m_sendSsrc = 0;
}

TypeId
SagRtcpHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::SagRtcpHeader")
            .SetParent<Header> ()
            .SetGroupName("Applications")
            .AddConstructor<SagRtcpHeader> ()
    ;
    return tid;
}

TypeId
SagRtcpHeader::GetInstanceTypeId () const
{
    NS_LOG_FUNCTION (this);
    return GetTypeId ();
}

uint32_t
SagRtcpHeader::GetSerializedSize () const
{
    NS_LOG_FUNCTION (this);
    return 1 + // First octet
            sizeof (m_packetType) +
            sizeof (m_length)     +
            sizeof (m_sendSsrc);
}

void
SagRtcpHeader::SerializeCommon (Buffer::Iterator& start) const
{
	NS_LOG_FUNCTION (this << &start);
    NS_ASSERT (m_typeOrCnt <= 0x1f);
    uint8_t octet1 = 0;
    octet1 |= (RTP_VERSION << 6);
    RtpHdrSetBit (octet1, 5, m_padding);
    octet1 |= uint8_t (m_typeOrCnt & 0x1f);
    start.WriteU8 (octet1);

    start.WriteU8 (m_packetType);
    start.WriteHtonU16 (m_length);
    start.WriteHtonU32 (m_sendSsrc);
}

void SagRtcpHeader::Serialize (Buffer::Iterator start) const
{
	NS_LOG_FUNCTION (this << &start);
    SerializeCommon (start);
}

uint32_t SagRtcpHeader::DeserializeCommon (Buffer::Iterator& start)
{
	NS_LOG_FUNCTION (this << &start);
    const auto octet1 = start.ReadU8 ();
    const uint8_t version = (octet1 >> 6);
    m_padding = RtpHdrGetBit (octet1, 5);
    m_typeOrCnt = octet1 & 0x1f;

    m_packetType = start.ReadU8 ();
    m_length = start.ReadNtohU16 ();
    m_sendSsrc = start.ReadNtohU32 ();
    NS_ASSERT (version == RTP_VERSION);
    return GetSerializedSize ();

}

uint32_t
SagRtcpHeader::Deserialize (Buffer::Iterator start)
{
	NS_LOG_FUNCTION (this << &start);
	return DeserializeCommon (start);
}

void
SagRtcpHeader::PrintN (std::ostream& os) const
{
	NS_LOG_FUNCTION (this << &os);
    os << "Rtcp Common Header - version = " << int (RTP_VERSION)
       << ", padding = " << (m_padding ? "yes" : "no")
       << ", type/count = " << int (m_typeOrCnt)
       << ", packet type = " << int (m_packetType)
       << ", length = " << m_length
       << ", ssrc of RTCP sender = " << m_sendSsrc;
}

void
SagRtcpHeader::Print (std::ostream& os) const
{
	NS_LOG_FUNCTION (this << &os);
    PrintN (os);
    os << std::endl;
}

bool
SagRtcpHeader::IsPadding () const
{
	NS_LOG_FUNCTION (this);
    return m_padding;
}

void
SagRtcpHeader::SetPadding (bool padding)
{
	NS_LOG_FUNCTION (this << padding);
    m_padding = padding;
}

uint8_t
SagRtcpHeader::GetTypeOrCount () const
{
	NS_LOG_FUNCTION (this);
    return m_typeOrCnt;
}

void
SagRtcpHeader::SetTypeOrCount (uint8_t typeOrCnt)
{
	NS_LOG_FUNCTION (this << typeOrCnt);
    m_typeOrCnt = typeOrCnt;
}

uint8_t
SagRtcpHeader::GetPacketType () const
{
	NS_LOG_FUNCTION (this);
    return m_packetType;
}

void
SagRtcpHeader::SetPacketType (uint8_t packetType)
{
	NS_LOG_FUNCTION (this << packetType);
    m_packetType = packetType;
}

uint32_t
SagRtcpHeader::GetSendSsrc () const
{
	NS_LOG_FUNCTION (this);
    return m_sendSsrc;
}

void
SagRtcpHeader::SetSendSsrc (uint32_t sendSsrc)
{
	NS_LOG_FUNCTION (this << sendSsrc);
    m_sendSsrc = sendSsrc;
}


constexpr uint16_t CCFeedbackHeader::MetricBlock::m_overrange;
constexpr uint16_t CCFeedbackHeader::MetricBlock::m_unavailable;

CCFeedbackHeader::CCFeedbackHeader ()
: SagRtcpHeader{RTP_FB, RTCP_RTPFB_CC}
, m_reportBlocks{}
, m_latestTsUs{0}
{
    ++m_length; // report timestamp field
}

CCFeedbackHeader::~CCFeedbackHeader () {}

void CCFeedbackHeader::Clear ()
{
    SagRtcpHeader::Clear ();
    m_packetType = RTP_FB;
    m_typeOrCnt = RTCP_RTPFB_CC;
    ++m_length; // report timestamp field
    m_reportBlocks.clear ();
    m_latestTsUs = 0;
}

TypeId CCFeedbackHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::CCFeedbackHeader")
            .SetParent<Header> ()
            .SetGroupName("BasicSim")
            .AddConstructor<CCFeedbackHeader> ()
    ;
    return tid;
}

TypeId
CCFeedbackHeader::GetInstanceTypeId () const
{
	NS_LOG_FUNCTION (this);
    return GetTypeId ();
}

CCFeedbackHeader::RejectReason
CCFeedbackHeader::AddFeedback (uint32_t ssrc, uint16_t seq, uint64_t timestampUs, uint8_t ecn)
{
    if (ecn > 0x03) {
        return CCFB_BAD_ECN;
    }
    auto& rb = m_reportBlocks[ssrc];
    if (rb.find (seq) != rb.end ()) {
        return CCFB_DUPLICATE;
    }
    auto& mb = rb[seq];
    mb.m_timestampUs = timestampUs;
    mb.m_ecn = ecn;
    if (!UpdateLength ()) {
        rb.erase (seq);
        if (rb.empty ()) {
            m_reportBlocks.erase (ssrc);
        }
        return CCFB_TOO_LONG;
    }
    m_latestTsUs = std::max (m_latestTsUs, timestampUs);
    return CCFB_NONE;
}

uint8_t
CCFeedbackHeader::Empty () const
{
	NS_LOG_FUNCTION (this);
    return m_reportBlocks.empty ();
}

void
CCFeedbackHeader::GetSsrcList (std::set<uint32_t>& rv) const
{
	NS_LOG_FUNCTION (this);
    rv.clear ();
    for (const auto& rb : m_reportBlocks) {
        rv.insert (rb.first);
    }
}

uint8_t
CCFeedbackHeader::GetMetricList (uint32_t ssrc,
                                      std::vector<std::pair<uint16_t, MetricBlock> >& rv) const
{
	NS_LOG_FUNCTION (this << ssrc << &rv);
    const auto it = m_reportBlocks.find (ssrc);
    if (it == m_reportBlocks.end ()) {
        return false;
    }
    rv.clear ();
    const auto& rb = it->second;
    NS_ASSERT (!rb.empty ()); // at least one metric block
    const auto beginStop = CalculateBeginStopSeq (rb);
    const uint16_t beginSeq = beginStop.first;
    const uint16_t stopSeq = beginStop.second;
    for (uint16_t i = beginSeq; i != stopSeq; ++i) {
        const auto& mb_it = rb.find (i);
        const bool received = (mb_it != rb.end ());
        if (received) {
            const auto item = std::make_pair (i, mb_it->second);
            rv.push_back (item);
        }
    }
    return true;
}

uint32_t
CCFeedbackHeader::GetSerializedSize () const
{
	NS_LOG_FUNCTION (this);
    NS_ASSERT (m_length >= 2);
    const auto commonHdrSize = SagRtcpHeader::GetSerializedSize ();
    return commonHdrSize + (m_length - 1) * 4;
}

void
CCFeedbackHeader::Serialize (Buffer::Iterator start) const
{
//	NS_LOG_FUNCTION (this >> &start);
    NS_ASSERT (m_length >= 2); // TODO (authors): 0 report blocks should be allowed
    SagRtcpHeader::SerializeCommon (start);

    NS_ASSERT (!m_reportBlocks.empty ()); // Empty reports are not allowed
    for (const auto& rb : m_reportBlocks) {
        start.WriteHtonU32 (rb.first);
        const auto beginStop = CalculateBeginStopSeq (rb.second);
        const uint16_t beginSeq = beginStop.first;
        const uint16_t stopSeq = beginStop.second;
        start.WriteHtonU16 (beginSeq);
        start.WriteHtonU16 (uint16_t (stopSeq - 1));
        NS_ASSERT (!rb.second.empty ()); // at least one metric block
        for (uint16_t i = beginSeq; i != stopSeq; ++i) {
            const auto& mb_it = rb.second.find (i);
            uint8_t octet1 = 0;
            uint8_t octet2 = 0;
            const bool received = (mb_it != rb.second.end ());
            RtpHdrSetBit (octet1, 7, received);
            if (received) {
                const auto& mb = mb_it->second;
                NS_ASSERT (mb.m_ecn <= 0x03);
                octet1 |= uint8_t ((mb.m_ecn & 0x03) << 5);
                const uint32_t ntp = UsToNtp (mb.m_timestampUs);
                const uint32_t ntpRef = UsToNtp (m_latestTsUs);
                const uint16_t ato = NtpToAto (ntp, ntpRef);
                NS_ASSERT (ato <= 0x1fff);
                octet1 |= uint8_t (ato >> 8);
                octet2 |= uint8_t (ato & 0xff);
            }
            start.WriteU8 (octet1);
            start.WriteU8 (octet2);
        }
        if (uint16_t (stopSeq - beginSeq) % 2 == 1) {
            start.WriteHtonU16 (0); //padding
        }
    }
    const uint32_t ntpTs = UsToNtp (m_latestTsUs);
    start.WriteHtonU32 (ntpTs);
}

uint32_t
CCFeedbackHeader::Deserialize (Buffer::Iterator start)
{
//	NS_LOG_FUNCTION (this >> &start);
    NS_ASSERT (m_length >= 2);
    (void) SagRtcpHeader::DeserializeCommon (start);
    NS_ASSERT (m_packetType == RTP_FB);
    NS_ASSERT (m_typeOrCnt == RTCP_RTPFB_CC);
    //length of all report blocks in 16-bit words
    size_t len_left = (size_t (m_length - 2 /* sender SSRC + Report Tstmp*/ )) * 2;
    while (len_left > 0) {
        NS_ASSERT (len_left >= 4); // SSRC + begin & end
        const auto ssrc = start.ReadNtohU32 ();
        auto& rb = m_reportBlocks[ssrc];
        const uint16_t beginSeq = start.ReadNtohU16 ();
        const uint16_t endSeq = start.ReadNtohU16 ();
        len_left -= 4;
        const uint16_t diff = endSeq - beginSeq; //this wraps properly
        const uint32_t nMetricBlocks = uint32_t (diff) + 1;
        NS_ASSERT (nMetricBlocks <= 0xffff);// length of 65536 not supported
        const uint32_t nPaddingBlocks = nMetricBlocks % 2;
        NS_ASSERT (len_left >= nMetricBlocks + nPaddingBlocks);
        uint16_t seq = beginSeq;
        for (auto i = 0u; i < nMetricBlocks; ++i) {
            const auto octet1 = start.ReadU8 ();
            const auto octet2 = start.ReadU8 ();
            if (RtpHdrGetBit (octet1, 7)) {
                uint16_t ato = (uint16_t (octet1) << 8) & 0x1f00;
                ato |= uint16_t (octet2);
                // 'Unavailable' treated as a lost packet
                if (ato != MetricBlock::m_unavailable) {
                    auto &mb = rb[seq];
                    mb.m_ecn = (octet1 >> 5) & 0x03;
                    mb.m_ato = ato;
                }
            }
            ++seq;
        }
        len_left -= nMetricBlocks;
        if (nPaddingBlocks == 1) {
            start.ReadNtohU16 (); //skip padding
            --len_left;
        }
    }
    // TODO (authors): "NTP timestamp field in RTCP Sender Report (SR) and Receiver Report (RR) packets"
    //                 (Minor) But, there's no NTP timestamp in RR packets
    const uint32_t ntpRef = start.ReadNtohU32 ();
    // Populate all timestamps once Report Timestamp is known
    // TODO (authors): Need second pass once RTS is deserialized
    for (auto& rb : m_reportBlocks) {
        for (auto& mb : rb.second) {
            const uint32_t ntp = AtoToNtp (mb.second.m_ato, ntpRef);
            mb.second.m_timestampUs = NtpToUs (ntp);
        }
    }
    m_latestTsUs = NtpToUs (ntpRef);
    NS_ASSERT (!m_reportBlocks.empty ()); // Empty reports are not allowed
    return GetSerializedSize ();
}

void
CCFeedbackHeader::Print (std::ostream& os) const
{
//	NS_LOG_FUNCTION (this >> &os);
    NS_ASSERT (m_length >= 2);
    SagRtcpHeader::PrintN (os);
    size_t i = 0;
    for (const auto& rb : m_reportBlocks) {
        const auto beginStop = CalculateBeginStopSeq (rb.second);
        const uint16_t beginSeq = beginStop.first;
        const uint16_t stopSeq = beginStop.second;
        os << ", report block #" << i << " = "
           << "{ SSRC = " << rb.first
           << " [" << beginSeq << ".." << uint16_t (stopSeq - 1) << "] --> ";
        for (uint16_t j = beginSeq; j != stopSeq; ++j) {
            const auto& mb_it = rb.second.find (j);
            const bool received = (mb_it != rb.second.end ());
            os << "<L=" << int (received);
            if (received) {
                const auto& mb = mb_it->second;
                const uint32_t ntp = UsToNtp (mb.m_timestampUs);
                const uint32_t ntpRef = UsToNtp (m_latestTsUs);
                os << ", ECN=0x" << std::hex << int (mb.m_ecn) << std::dec
                   << ", ATO=" << NtpToAto (ntp, ntpRef);
            }
            os << ">,";
        }
        os << " }, ";
        ++i;
    }
    os << "RTS = " << UsToNtp (m_latestTsUs) << std::endl;
}

std::pair<uint16_t, uint16_t>
CCFeedbackHeader::CalculateBeginStopSeq (const ReportBlock_t& rb)
{
//	NS_LOG_FUNCTION (this >> &rb);
    NS_ASSERT (!rb.empty ()); // at least one metric block
    auto mb_it = rb.begin ();
    const uint16_t first = mb_it->first;
    if (rb.size () == 1) {
        return std::make_pair (first, first + 1);
    }
    //calculate biggest gap
    uint16_t low = first;
    ++mb_it;
    uint16_t high = mb_it->first;
    uint16_t max_lo = low;
    uint16_t max_hi = high;
    ++mb_it;
    for (; mb_it != rb.end (); ++mb_it) {
        low = high;
        high = mb_it->first;
        NS_ASSERT (low < high);
        NS_ASSERT (max_lo < max_hi);
        if ((high - low) > (max_hi - max_lo)) {
            max_lo = low;
            max_hi = high;
        }
    }
    //check the gap across wrapping
    NS_ASSERT (max_lo < max_hi);
    if (uint16_t (first - high) > (max_hi - max_lo)) {
        max_lo = high;
        max_hi = first;
    }
    ++max_lo;
    NS_ASSERT (max_hi != max_lo); // length of 65536 not supported
    return std::make_pair (max_hi, max_lo);
}

uint8_t
CCFeedbackHeader::UpdateLength ()
{
	NS_LOG_FUNCTION (this);
    size_t len = 1; // SSRC of packet sender
    for (const auto& rb : m_reportBlocks) {
        ++len; // SSRC
        ++len; // begin & end seq
        const auto beginStop = CalculateBeginStopSeq (rb.second);
        const uint16_t beginSeq = beginStop.first;
        const uint16_t stopSeq = beginStop.second;
        const uint16_t nMetricBlocks = stopSeq - beginSeq; //this wraps properly
        const uint16_t nPaddingBlocks = nMetricBlocks % 2;
        len += (nMetricBlocks + nPaddingBlocks) / 2; // metric blocks are 16 bits long
    }
    ++len; // report timestamp field
    if (len > 0xffff) {
        return false;
    }
    m_length = len;
    return true;
}

uint16_t
CCFeedbackHeader::NtpToAto (uint32_t ntp, uint32_t ntpRef)
{
//	NS_LOG_FUNCTION (this >> ntp >>ntpRef);
    NS_ASSERT (ntp <= ntpRef);
    // ato contains offset measured in 1/1024 seconds
    const uint32_t atoNtp = ntpRef - ntp;
    const uint32_t atoNtpRounded = atoNtp + (1 << 5);
    const uint16_t ato = uint16_t (atoNtpRounded >> 6); // i.e., * 0x400 / 0x10000
    return std::min (ato, MetricBlock::m_overrange);
}

uint32_t
CCFeedbackHeader::AtoToNtp (uint16_t ato, uint32_t ntpRef)
{
//	NS_LOG_FUNCTION (this >> ato >>ntpRef);
    NS_ASSERT (ato < MetricBlock::m_unavailable);
    // ato contains offset measured in 1/1024 seconds
    const uint32_t atoNtp = uint32_t (ato) << 6; // i.e., * 0x10000 / 0x400
    NS_ASSERT (atoNtp <= ntpRef);
    return ntpRef - atoNtp;
}

uint64_t
CCFeedbackHeader::NtpToUs (uint32_t ntp)
{
//	NS_LOG_FUNCTION (this >> ntp);
    const double tsSeconds = double (ntp) / double (0x10000);
    return uint64_t (tsSeconds * 1000. * 1000. *1000.); //* 1000. * 1000.
}

uint32_t
CCFeedbackHeader::UsToNtp (uint64_t tsUs)
{
//	NS_LOG_FUNCTION (this >> tsUs);
    const double tsSeconds = double (tsUs) / 1000. / 1000. / 1000.; /// 1000. / 1000.
    return uint32_t (tsSeconds * double (0x10000));
}

} // namespace ns3
