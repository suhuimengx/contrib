
#ifndef TLR_QUEUE_H
#define TLR_QUEUE_H

#include <vector>
#include <queue>
#include <string>
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/simulator.h"

namespace ns3 {
    namespace tlr {

        /**
         * \ingroup tlr
         * \brief tlr Queue Entry
         */
        class QueueEntry
        {
        public:
            /// IPv4 routing unicast forward callback typedef
            typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
            /// IPv4 routing error callback typedef
            typedef Ipv4RoutingProtocol::LocalDeliverCallback LocalDeliverCallback;
            /**
             * constructor
             *
             * \param pa the packet to add to the queue
             * \param h the Ipv4Header
             * \param ucb the UnicastForwardCallback function
             * \param ecb the ErrorCallback function
             */
            QueueEntry(Ptr<const Packet> pa = 0, Ipv4Header const& h = Ipv4Header(),
            	Ipv4Address nxthop = 0, uint32_t k = 0,
                UnicastForwardCallback ucb = UnicastForwardCallback(),
				LocalDeliverCallback lcb = LocalDeliverCallback(), int32_t iif=0 )
                : m_packet(pa),
                  m_header(h),
				  m_nxthop(nxthop),
                  m_nxthopid(k),
                  m_ucb(ucb),
                  m_lcb(lcb),
				  m_iif(iif)
            {
            

            }

            UnicastForwardCallback GetUnicastForwardCallback () const
            {
                return m_ucb;
            }

            void SetUnicastForwardCallback (UnicastForwardCallback ucb)
            {
                m_ucb = ucb;
            }

            LocalDeliverCallback GetLocalDeliverCallback () const
            {
                return m_lcb;
            }

            void SetLocalDeliverCallback (LocalDeliverCallback lcb)
            {
                m_lcb = lcb;
            }

            Ptr<const Packet> GetPacket() const
            {
                return m_packet;
            }
            /**
             * Set packet in entry
             * \param p The packet
             */
            void SetPacket(Ptr<const Packet> p)
            {
                m_packet = p;
            }
            /**
             * Get IPv4 header
             * \returns the IPv4 header
             */
            Ipv4Header GetIpv4Header() const
            {
                return m_header;
            }
            /**
             * Set IPv4 header
             * \param h the IPv4 header
             */
            void SetIpv4Header(Ipv4Header h)
            {
                m_header = h;
            }
            uint8_t GetTTW() const
            {
                return m_ttw;
            }
            /**
            * Set ttw
            * \param p the ttw
            */
            void SetTTW(uint8_t p)
            {
                m_ttw = p;
            }
            Ipv4Address GetNextHop() const
            {
                return m_nxthop;
            }
            uint32_t GetNextHopId() const
            {
                return m_nxthopid;
            }

            /**
            * Set nxthopid
            * \param p the nxthopid
            */
            void SetNextHopId(uint32_t p)
            {
                m_nxthopid = p;
            }
            void SetNextHopId(Ipv4Address ad)
            {
                 m_nxthop = ad;
            }
            int32_t GetIIF() const
            {
                return m_iif;
            }
        private:
            /// Data packet
            Ptr<const Packet> m_packet;
            /// IP header
            Ipv4Header m_header;
            Ipv4Address m_nxthop;
            uint32_t m_nxthopid;
            /// Unicast forward callback
           // UnicastForwardCallback m_ucb;
            /// Error callback
          //  ErrorCallback m_ecb;
            /// Expire time for queue entry
            //Time m_expire;
            ///Times to wait 
            uint8_t m_ttw;
            UnicastForwardCallback m_ucb;
            LocalDeliverCallback m_lcb;
            int32_t m_iif;
        };
        /**
         * \ingroup tlr
         * \brief TLR route request queue
         *
         * Since TLR is an on demand routing we queue requests while looking for route.
         */
        class TlrQueue
        {
        public:
            /**
             * constructor
             *
             * \param maxLen the maximum length
             * \param routeToQueueTimeout the route to queue timeout
             */
            TlrQueue(uint32_t maxLen)
                : m_maxLen(maxLen)
            {

            }
            /**
             * Push entry in queue
             * \param entry the queue entry
             * \returns true if the entry is queued
             */
            void Enqueue(QueueEntry& entry);
            std::vector<QueueEntry>::iterator Delete(std::vector<QueueEntry>::iterator i);
            std::vector<QueueEntry>::iterator UpdateTTW(std::vector<QueueEntry>::iterator i);

            //void Check(std::vector<Ipv4Address> nownb,std::map<Ipv4Address, uint8_t> nowtrafficlightcolor);
            uint32_t GetSize();

            // Fields
            /**
             * Get maximum queue length
             * \returns the maximum queue length
             */
            uint32_t GetMaxQueueLen() const
            {
                return m_maxLen;
            }
            /**
             * Set maximum queue length
             * \param len The maximum queue length
             */
            void SetMaxQueueLen(uint32_t len)
            {
                m_maxLen = len;
            }

            std::vector<QueueEntry> GetQueue()
			{
            	return m_queue;
			}
            /// The queue
            std::vector<QueueEntry> m_queue;
        private:


            /// Remove all expired entries
            void Purge();
            /**
             * Notify that packet is dropped from queue by timeout
             * \param en the queue entry to drop
             * \param reason the reason to drop the entry
             */
            void Drop(QueueEntry en, std::string reason);
            /// The maximum number of packets that we allow a routing protocol to buffer.
            uint32_t m_maxLen;
        };


    }  // namespace tlr
}  // namespace ns3

#endif /* TLR_RQUEUE_H */
