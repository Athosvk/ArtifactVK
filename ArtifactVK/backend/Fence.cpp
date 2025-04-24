#include "Fence.h"

#include <stdexcept>

Fence::Fence(VkDevice device) : Fence(device, false)
{
}

Fence::Fence(VkDevice device, bool startSignaled) : m_Device(device)
{
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (startSignaled)
    {
        createInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
    }

    if (vkCreateFence(device, &createInfo, nullptr, &m_Fence) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create fence");
    }
}

Fence::Fence(Fence &&other) : m_Device(other.m_Device), m_Fence(std::exchange(other.m_Fence, VK_NULL_HANDLE))
{

}

Fence::~Fence()
{
    if (m_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_Device, m_Fence, nullptr);
    }
}

void Fence::Wait()
{
    vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(m_Device, 1, &m_Fence);
}
