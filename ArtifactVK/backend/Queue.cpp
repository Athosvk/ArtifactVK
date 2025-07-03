#include "Queue.h"

Queue::Queue(VkDevice device, uint32_t queueFamilyIndex) : m_QueueFamilyIndex(queueFamilyIndex)
{
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &m_Queue);
}

VkQueue Queue::Get() const
{
    return m_Queue;
}

void Queue::Wait() const
{
    vkQueueWaitIdle(m_Queue);
}

uint32_t Queue::GetFamilyIndex() const
{
    return m_QueueFamilyIndex;
}
