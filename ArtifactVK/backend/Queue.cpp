#include "Queue.h"

Queue::Queue(VkDevice device, uint32_t queueIndex)
{
    vkGetDeviceQueue(device, queueIndex, 0, &m_Queue);
}

VkQueue Queue::Get() const
{
    return m_Queue;
}
