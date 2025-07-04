#include "CommandBufferPool.h"
#include "VulkanDevice.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Pipeline.h"
#include "Barrier.h"

#include <cassert>
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <iostream>
#include <span>

CommandBuffer::CommandBuffer(VkCommandBuffer &&commandBuffer, VkDevice device, Queue queue) : m_CommandBuffer(commandBuffer), 
    // Start the Fence signaled so that we can query for correct usage prior to beginning the command buffer (again)
      m_InFlight(Fence(device)), m_Queue(queue)
{
}

CommandBuffer::CommandBuffer(CommandBuffer &&other)
    : m_CommandBuffer(other.m_CommandBuffer), 
    m_InFlight(std::move(other.m_InFlight)), m_Status(other.m_Status), 
    m_Queue(other.m_Queue), m_PendingBarriers(std::move(other.m_PendingBarriers))
{
    other.m_Moved = true;
}

CommandBuffer::~CommandBuffer()
{
    if (!m_Moved)
    {
        assert(m_InFlight.QueryStatus() != FenceStatus::UnsignaledOrReset &&
               "Attempting to delete a command buffer that is still in flight."
               "Wait for the returned fence in `CommandBuffer::End`");
    }
}

void CommandBuffer::WaitFence()
{
    // No use in waiting for a fence that cannot possibly have been signaled
    if (m_Status != CommandBufferStatus::Reset) 
    {
        m_InFlight.WaitAndReset();
    }
}

// TODO: Consider doing begin on first command invocation
void CommandBuffer::Begin()
{
    assert(m_Status != CommandBufferStatus::Recording && "Command buffer already recording");
    assert(m_InFlight.WasReset() && "Attempting to begin a command buffer that may still be in flight. Wait for the returned fence");
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
    m_Status = CommandBufferStatus::Recording;
}

void CommandBuffer::Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline, 
    VertexBuffer& vertexBuffer, const UniformBuffer& uniformBuffer)
{
    assert(m_Status == CommandBufferStatus::Recording && "Calling draw before starting recording of command buffer");
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
    BindVertexBuffer(vertexBuffer);
    BindUniformBuffer(uniformBuffer, pipeline);
    // TODO: Give user control over what to draw

    for (const auto &barrierArray : m_PendingBarriers)
    {
        InsertBarriers(barrierArray);
    }
    m_PendingBarriers.clear();
    vkCmdDraw(m_CommandBuffer, static_cast<uint32_t>(vertexBuffer.VertexCount()), 1, 0, 0);
    vkCmdEndRenderPass(m_CommandBuffer);
}

void CommandBuffer::DrawIndexed(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline,
    VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, const UniformBuffer& uniformBuffer)
{
    assert(m_Status == CommandBufferStatus::Recording && "Calling draw before starting recording of command buffer");
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
    BindVertexBuffer(vertexBuffer);
    BindIndexBuffer(indexBuffer);
    BindUniformBuffer(uniformBuffer, pipeline);

    for (const auto &barrierArray : m_PendingBarriers)
    {
        InsertBarriers(barrierArray);
    }
    m_PendingBarriers.clear();
    // TODO: Give user control over what to draw
    vkCmdDrawIndexed(m_CommandBuffer, static_cast<uint32_t>(indexBuffer.GetIndexCount()), 1, 0, 0, 0);
    vkCmdEndRenderPass(m_CommandBuffer);
}

// TODO: Bind command buffer to a queue at creation time
Fence& CommandBuffer::End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores)
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
    auto res = vkQueueSubmit(m_Queue.Get(), 1, &submitInfo, m_InFlight.Get());
    if (res != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Faied to submit cmd buffer to queue");
    }
    m_Status = CommandBufferStatus::Submitted;
    return m_InFlight;
}

Fence &CommandBuffer::End()
{
    return End(std::span<Semaphore>(), std::span<Semaphore>());
}

void CommandBuffer::BindVertexBuffer(VertexBuffer &vertexBuffer)
{
    auto& buffer = vertexBuffer.GetBuffer();
    VkBuffer vertexBuffers = {buffer.Get()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vertexBuffers, offsets);

    HandleAcquire(buffer);
}

void CommandBuffer::BindIndexBuffer(IndexBuffer &indexBuffer)
{
    auto& buffer = indexBuffer.GetBuffer();
    VkBuffer indexBuffers = {buffer.Get()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffers, 0, VkIndexType::VK_INDEX_TYPE_UINT16);
    HandleAcquire(buffer);
}

void CommandBuffer::BindUniformBuffer(const UniformBuffer &uniformBuffer, const RasterPipeline& pipeline)
{
    auto descriptorSet = uniformBuffer.GetDescriptorSet();
    auto pipelineLayout = pipeline.GetPipelineLayout();
    vkCmdBindDescriptorSets(m_CommandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
}

void CommandBuffer::HandleAcquire(DeviceBuffer &buffer)
{
    auto pendingAcquire = buffer.TakePendingAcquire();
    if (pendingAcquire.has_value())
    {
        auto array = std::find_if(m_PendingBarriers.begin(), m_PendingBarriers.end(),
                     [&pendingAcquire](const BufferMemoryBarrierArray &array) {
                         return array.DestinationStageMask == pendingAcquire->DestinationStageMask &&
                                array.SourceStageMask == pendingAcquire->SourceStageMask;
                     });
        if (array != m_PendingBarriers.end())
        {
            array->Barriers.emplace_back(std::move(pendingAcquire->Barrier));
        }
        else
        {
            m_PendingBarriers.emplace_back(BufferMemoryBarrierArray(std::move(*pendingAcquire)));
        }
    }
}

void CommandBuffer::Copy(const DeviceBuffer &source, const DeviceBuffer &destination)
{
    // TODO: Needs to handle pending acquires?
    // TODO: Use `Begin` instead and allow for a one-time fire
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    VkBufferCopy bufferCopy{};
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = source.GetSize();
    vkCmdCopyBuffer(m_CommandBuffer, source.Get(), destination.Get(), 1, &bufferCopy);
}

void CommandBuffer::CopyBufferToImage(const DeviceBuffer &source, const Texture &texture)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;

    bufferImageCopy.imageSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;

    bufferImageCopy.imageOffset = {0, 0, 0};
    bufferImageCopy.imageExtent = {texture.GetWidth(), texture.GetHeight(), 1};

    vkCmdCopyBufferToImage(m_CommandBuffer, source.Get(), texture.Get(),
                           VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
}

void CommandBuffer::InsertBarrier(const BufferMemoryBarrier &barrier) const
{
    VkBufferMemoryBarrier vkBarrier{};
    vkBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vkBarrier.buffer = barrier.Barrier.Buffer.get().Get();
    vkBarrier.srcAccessMask = barrier.Barrier.SourceAccessMask;
    vkBarrier.dstAccessMask = barrier.Barrier.DestinationAccessMask;
    vkBarrier.srcQueueFamilyIndex = barrier.Barrier.SourceQueue.GetFamilyIndex();
    vkBarrier.dstQueueFamilyIndex = barrier.Barrier.DestinationQueue.GetFamilyIndex();
    vkBarrier.offset = 0;
    vkBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(m_CommandBuffer, barrier.SourceStageMask, barrier.DestinationStageMask, 0, 0, nullptr, 1,
                         &vkBarrier, 0, nullptr);
}

void CommandBuffer::InsertBarrier(const ImageMemoryBarrier &barrier) const
{
    VkImageMemoryBarrier memoryBarrier;
    memoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.oldLayout = barrier.SourceLayout;
    memoryBarrier.newLayout = barrier.DestinationLayout;
    memoryBarrier.srcQueueFamilyIndex = barrier.SourceQueue.GetFamilyIndex();
    memoryBarrier.dstQueueFamilyIndex = barrier.DestinationQueue.GetFamilyIndex();
    memoryBarrier.image = barrier.Image.get().Get();

    memoryBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.subresourceRange.levelCount = 1;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.layerCount = 1;

    memoryBarrier.srcAccessMask = barrier.SourceAccessMask;
    memoryBarrier.dstAccessMask = barrier.DestinationAccessMask;
    vkCmdPipelineBarrier(m_CommandBuffer, barrier.SourceStageMask, barrier.DestinationAccessMask, 0, 0, nullptr, 0,
                         nullptr, 1, &memoryBarrier);
}

void CommandBuffer::InsertBarriers(const BufferMemoryBarrierArray &barriers) const
{
    std::vector<VkBufferMemoryBarrier> vkBarriers{};
    vkBarriers.reserve(barriers.Barriers.size());

    for (auto &barrier : barriers.Barriers)
    {
		VkBufferMemoryBarrier vkBarrier{};
		vkBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkBarrier.buffer = barrier.Buffer.get().Get();
		vkBarrier.srcAccessMask = barrier.SourceAccessMask;
		vkBarrier.dstAccessMask = barrier.DestinationAccessMask;
		vkBarrier.srcQueueFamilyIndex = barrier.SourceQueue.GetFamilyIndex();
		vkBarrier.dstQueueFamilyIndex = barrier.DestinationQueue.GetFamilyIndex();
		vkBarrier.offset = 0;
		vkBarrier.size = VK_WHOLE_SIZE;
		vkBarriers.push_back(vkBarrier);
    }

    vkCmdPipelineBarrier(m_CommandBuffer, barriers.SourceStageMask, barriers.DestinationStageMask, 0, 0, nullptr, static_cast<uint32_t>(vkBarriers.size()),
                         vkBarriers.data(), 0, nullptr);
}

Queue CommandBuffer::GetQueue() const
{
    return m_Queue;
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

std::vector<std::reference_wrapper<CommandBuffer>> CommandBufferPool::CreateCommandBuffers(uint32_t count, Queue queue)
{
    VkCommandBufferAllocateInfo allocationInfo{};
    allocationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandBufferCount = count;
    allocationInfo.commandPool = m_CommandBufferPool;
    allocationInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    std::vector<VkCommandBuffer> commandBuffers(count);

    if (vkAllocateCommandBuffers(m_Device, &allocationInfo, commandBuffers.data()) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    
    std::vector<std::reference_wrapper<CommandBuffer>> commandBufferHandles;
    for (auto&& vkCommandBuffer : commandBuffers)
    {
        commandBufferHandles.emplace_back(*m_CommandBuffers.emplace_back(std::make_unique<CommandBuffer>(std::move(vkCommandBuffer), m_Device, queue)));
    }

    return commandBufferHandles;
}

CommandBuffer &CommandBufferPool::CreateCommandBuffer(Queue queue)
{
    return CreateCommandBuffers(1, queue)[0];
}
