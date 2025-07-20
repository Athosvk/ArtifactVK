#include "Fence.h"

#include <stdexcept>
#include <iostream>

#include "DebugMarker.h"

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
        m_Status = FenceStatus::Signaled;
    } 
    else
    {
        m_Status = FenceStatus::Reset;
    }

    if (vkCreateFence(device, &createInfo, nullptr, &m_Fence) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create fence");
    }
}

Fence::Fence(Fence &&other)
    : m_Device(other.m_Device), m_Fence(std::exchange(other.m_Fence, VK_NULL_HANDLE)), m_Status(other.m_Status)
{

}

Fence::~Fence()
{
    if (m_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_Device, m_Fence, nullptr);
    }
}

void Fence::WaitAndReset()
{
    // TODO: Don't insert wait into the created device through this, it's not obvious
    // that this will insert into the command buffer. This should probably be a function on
    // the device or cmd buffer
    auto result = vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    if (result != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Wait unsuccesful");
    }
    vkResetFences(m_Device, 1, &m_Fence);
    m_Status = FenceStatus::Reset;
}

VkFence Fence::Get() const
{
    // Cannot know whether the fence maybe enter an unsignaled state,
    // depends on the usage outside of this.
    m_Status = FenceStatus::UnsignaledOrReset;
    return m_Fence;
}

FenceStatus Fence::QueryStatus()
{
    // We know it has to be in its reset (unsignaled) state
    // if the last call was to `this::WaitAndReset()`. The only exception
    // is abusing `this::Get()` by calling `this::WaitAndReset()` while the caller
    // retains the obtained handle to the underlying `VkFence`
    // TODO: Consider using a delegator-based pattern instead of `Get`
    // to guarantee correct usage. Or hack in a scope guard with private 
    // constructors etc. so that people can never move/copy them out of a function
    // call
    if (m_Status == FenceStatus::Reset)
    {
        return m_Status;
    }
    VkResult result = vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, 0);
    if (result == VkResult::VK_SUCCESS)
    {
        m_Status = FenceStatus::Signaled;
    }
    else if (result == VkResult::VK_TIMEOUT)
    {
        m_Status = FenceStatus::UnsignaledOrReset;
    }
    else
    {
        throw std::runtime_error("Fence wait failed");
    }
    return m_Status;
}

bool Fence::WasReset() const
{
    return m_Status == FenceStatus::Reset;
}

void Fence::SetName(const std::string& name, const ExtensionFunctionMapping& functionMapping) const
{
    DebugMarker::SetName(m_Device, functionMapping, m_Fence, name);
}
