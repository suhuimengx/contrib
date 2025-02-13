
#include "fybbr-queue.h"

#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

namespace ns3 {
namespace fybbr {

uint32_t
FYBBRServerQueue::GetSize()
{
	return m_queue.size();
}

void
FYBBRServerQueue::Enqueue(QueueEntry entry)
{

	if (m_queue.size() == m_maxLen)
	{
		throw std::runtime_error("FYBBR: No enough controller queue length.");
	}
	m_queue.push_back(entry);

}

void
FYBBRServerQueue::Dequeue (QueueEntry & entry)
{
	if (m_queue.size() == 0)
	{
		throw std::runtime_error("FYBBR: No entry.");
	}
	entry = *m_queue.begin ();
	m_queue.erase(m_queue.begin ());
}

}
}
