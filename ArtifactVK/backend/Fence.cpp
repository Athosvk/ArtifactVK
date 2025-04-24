#include "Fence.h"

#include <stdexcept>

Fence::Fence(VkDevice device) : m_Device(device)
{
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

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
