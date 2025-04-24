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

Fence::~Fence()
{
    vkDestroyFence(m_Device, m_Fence, nullptr);
}
