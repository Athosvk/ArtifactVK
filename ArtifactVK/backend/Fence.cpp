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
    // TODO: Don't insert wait into the created device through this, it's not obvious
    // that this will insert into the command buffer. This should probably be a function on
    // the device or cmd buffer
    vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(m_Device, 1, &m_Fence);
}

VkFence Fence::Get() const
{
    return m_Fence;
}

bool Fence::QuerySignaled() const
{
    VkResult result = vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, 0);
    if (result == VkResult::VK_SUCCESS)
    {
        return true;
    }
    else if (result == VkResult::VK_TIMEOUT)
    {
        return false;
    }
    else
    {
        throw std::runtime_error("Fence wait failed");
    }
}
