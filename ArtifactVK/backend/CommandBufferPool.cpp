#include "CommandBufferPool.h"
#include "VulkanDevice.h"
#include "Framebuffer.h"
#include "RenderPass.h"

#include <cassert>
#include <vulkan/vulkan.h>
#include <stdexcept>

CommandBuffer::CommandBuffer(VkCommandBuffer &&commandBuffer) : m_CommandBuffer(commandBuffer)
{
}

void CommandBuffer::Begin()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(m_CommandBuffer, &beginInfo) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not begin command buffer");
    }
}

void CommandBuffer::Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, 
    const RasterPipeline& pipeline)
{
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = frameBuffer.Get();
    renderPassBeginInfo.renderPass = renderPass.Get();

    auto viewport = frameBuffer.GetViewport();
    // Should only be rendering to the scissor area, not the entire viewport
    renderPassBeginInfo.renderArea = viewport.Scissor;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
    pipeline.Bind(m_CommandBuffer, viewport);
    // TODO: Give user control over what to draw
    vkCmdDraw(m_CommandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(m_CommandBuffer);
}

void CommandBuffer::End()
{
    if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not end command buffer");
    }
}

CommandBufferPool::CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo) : m_Device(device)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = createInfo.CreationFlags;
    commandPoolCreateInfo.queueFamilyIndex = createInfo.QueueIndex;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &m_CommandBufferPool))
    {
        throw std::runtime_error("Could not create CommandBufferPool for queue index " + std::to_string(createInfo.QueueIndex));
    }
}

CommandBufferPool::CommandBufferPool(CommandBufferPool &&other) : 
    m_Device(other.m_Device),
    m_CommandBufferPool(std::exchange(other.m_CommandBufferPool, VK_NULL_HANDLE))
{
}

CommandBufferPool::~CommandBufferPool()
{
    vkDestroyCommandPool(m_Device, m_CommandBufferPool, nullptr);
}

CommandBuffer &CommandBufferPool::CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo allocationInfo{};
    allocationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandBufferCount = 1;
    allocationInfo.commandPool = m_CommandBufferPool;
    allocationInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    
    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(m_Device, &allocationInfo, &commandBuffer) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    return m_CommandBuffers.emplace_back(std::move(commandBuffer));
}
