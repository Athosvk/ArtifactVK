#include "CommandBufferPool.h"
#include "VulkanDevice.h"
#include "Framebuffer.h"
#include "RenderPass.h"

#include <cassert>
#include <vulkan/vulkan.h>
#include <stdexcept>

CommandBuffer::CommandBuffer(VkCommandBuffer &&commandBuffer, VkDevice device) : m_CommandBuffer(commandBuffer), 
    // Start the Fence signaled so that we can query for correct usage prior to beginning the command buffer (again)
    m_InFlight(Fence(device))
{
}

CommandBuffer::~CommandBuffer()
{
    assert(m_InFlight.QuerySignaled() && "Attempting to delete a command buffer that is still in flight."
        "Wait for the returned fence in `CommandBuffer::End`");
}

void CommandBuffer::Begin()
{
    assert(m_InFlight.WasReset() && "Attempting to begin a command buffer that may still be in fligh. Wait for the returned fence");
    if (m_Status == CommandBufferStatus::Submitted)
    {
        // Reset in case this command buffer was previously submitted
        Reset();
    }
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


Fence& CommandBuffer::End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores, VkQueue queue)
{
    if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not end command buffer");
    }
    
    // TODO: Ugly allocation, cache this somehow? Or reinterpret_cast
    // this somehow
    std::vector<VkSemaphore> waitSemaphoreHandles;
    waitSemaphoreHandles.reserve(waitSemaphores.size());
    for (const auto& semaphore : waitSemaphores)
    {
        waitSemaphoreHandles.emplace_back(semaphore.Get());
    }

    // TODO: Ugly allocation, cache this somehow? Or reinterpret_cast
    // this somehow
    std::vector<VkSemaphore> signalSemaphoreHandles;
    signalSemaphoreHandles.reserve(signalSemaphores.size());
    for (const auto& semaphore : signalSemaphores)
    {
        signalSemaphoreHandles.emplace_back(semaphore.Get());
    }

    // TODO: Expose to caller, next: cache from last inserted command
    // in previous cmd buffer
    VkPipelineStageFlags waitStages[] = {VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphoreHandles.size());
    submitInfo.pWaitSemaphores = waitSemaphoreHandles.data();
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphoreHandles.size());
    submitInfo.pSignalSemaphores = signalSemaphoreHandles.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffer;
    if (vkQueueSubmit(queue, 1, &submitInfo, m_InFlight.Get()) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("Faied to submit cmd buffer to queue");
    }
    m_Status = CommandBufferStatus::Submitted;
    return m_InFlight;
}

void CommandBuffer::Reset()
{
    vkResetCommandBuffer(m_CommandBuffer, 0);
    m_Status = CommandBufferStatus::Reset;
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
    m_CommandBufferPool(std::exchange(other.m_CommandBufferPool, VK_NULL_HANDLE)),
    m_CommandBuffers(std::move(other.m_CommandBuffers))
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
    return m_CommandBuffers.emplace_back(std::move(commandBuffer), m_Device);
}
