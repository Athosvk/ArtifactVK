#include "Semaphore.h"

#include <stdexcept>

Semaphore::Semaphore(VkDevice device) : 
    m_Device(device)
{
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &createInfo, nullptr, &m_Semaphore) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create semaphore");
    }
}

Semaphore::Semaphore(Semaphore &&other) : m_Device(other.m_Device), m_Semaphore(std::exchange(other.m_Semaphore, VK_NULL_HANDLE))
{
}

Semaphore::~Semaphore()
{
    if (m_Semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_Device, m_Semaphore, nullptr);
    }
}
