
#include "tlr-queue.h"
#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("TlrQueue");

    namespace tlr {
        uint32_t
            TlrQueue::GetSize()
        {
            return m_queue.size();
        }

       void
            TlrQueue::Enqueue(QueueEntry & entry)
        {
            
            entry.SetTTW(20);
            if (m_queue.size() == m_maxLen)
            {
                Drop(m_queue.front(), "Out of Queue Maxsize Drop the most aged packet"); // Drop the most aged packet
                m_queue.erase(m_queue.begin());
            }
            m_queue.push_back(entry);
   
        }

       std::vector<QueueEntry>::iterator
	    TlrQueue::Delete(std::vector<QueueEntry>::iterator i)
       {
           return m_queue.erase(i);
       }

       std::vector<QueueEntry>::iterator
	       TlrQueue::UpdateTTW(std::vector<QueueEntry>::iterator i )
       {
    	   uint8_t t = m_queue[i-m_queue.begin()].GetTTW();
    	   t=t-1;
    	    if (t == 0) {
    	       	Drop(m_queue[i-m_queue.begin()], "TTW =0");
    	        return m_queue.erase(i);
    	       	   }
    	     else m_queue[i-m_queue.begin()].SetTTW(t);
    	    return  i ;
       }


        void
            TlrQueue::Drop(QueueEntry en, std::string reason)
        {
            NS_LOG_LOGIC(reason << en.GetPacket()->GetUid() << " " << en.GetIpv4Header().GetDestination());
            //en.GetErrorCallback() (en.GetPacket(), en.GetIpv4Header(),
                //Socket::ERROR_NOROUTETOHOST);
            return;
        }

    }  // namespace tlr
}  // namespace ns3
