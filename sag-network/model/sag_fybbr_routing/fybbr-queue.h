
#ifndef FYBBR_QUEUE_H
#define FYBBR_QUEUE_H

#include <vector>
#include <queue>
#include <string>
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/fybbr_link_state_packet.h"
#include "ns3/fybbr-packet.h"

namespace ns3 {
    namespace fybbr {

        /**
         * \ingroup fybbr
         * \brief fybbr Queue Entry
         */
        class QueueEntry
        {
        public:
        	QueueEntry(){

        	}

            QueueEntry(std::pair<RTSHeader,RTSPacket> rts)
            {
            	m_rts = rts;
            }

            std::pair<RTSHeader,RTSPacket> GetRTS(){
            	return m_rts;
            }

        private:
            std::pair<RTSHeader,RTSPacket> m_rts;

        };
        /**
         * \ingroup fybbr
         * \brief FYBBR server queue
         *
         */
        class FYBBRServerQueue
        {
        public:
            /**
             * constructor
             *
             * \param maxLen the maximum length
             * \param routeToQueueTimeout the route to queue timeout
             */
        	FYBBRServerQueue(){

        	}
        	FYBBRServerQueue(uint32_t maxLen)
                : m_maxLen(maxLen)
            {

            }
            /**
             * Push entry in queue
             * \param entry the queue entry
             * \returns true if the entry is queued
             */
            void Enqueue(QueueEntry entry);
            void Dequeue (QueueEntry & entry);


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

        private:

            uint32_t m_maxLen;
            std::vector<QueueEntry> m_queue;
        };


    }  // namespace fybbr
}  // namespace ns3

#endif /* FYBBR_QUEUE_H */
