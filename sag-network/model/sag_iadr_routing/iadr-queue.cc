
#include "iadr-queue.h"

#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {
namespace iadr {

uint32_t
IADRServerQueue::GetSize()
{
	return m_queue.size();
}

void
IADRServerQueue::Enqueue(QueueEntry entry)
{

	if (m_queue.size() == m_maxLen)
	{
		throw std::runtime_error("IADR: No enough controller queue length.");
	}
	m_queue.push_back(entry);

}

void
IADRServerQueue::Dequeue (QueueEntry & entry)
{
	if (m_queue.size() == 0)
	{
		throw std::runtime_error("IADR: No entry.");
	}
	entry = *m_queue.begin ();
	m_queue.erase(m_queue.begin ());
}

}
}
