#include <backend/Timer.h>

#include <backend/TimerPool.h>

Timer::Timer(VkCommandBuffer commandBuffer, TimerPool &timerPool, TimestampId timestampId) : 
    m_TimestampId(timestampId), 
    m_TimerPool(timerPool), 
    m_CommandBuffer(commandBuffer)
{
	// Should not use anything else than bottom of pipe bit, results within passes
	// are unreliable.
    // Bottom of pipe should be correct here: we're waiting for the previous command
    // to be flushed.
	vkCmdWriteTimestamp(m_CommandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timerPool.GetQueryPool(), m_TimestampId.Begin);
}

Timer::Timer(Timer &&other) : 
    m_TimerPool(other.m_TimerPool), m_TimestampId(other.m_TimestampId), 
    m_CommandBuffer(std::exchange(other.m_CommandBuffer, VK_NULL_HANDLE))
{
}
 
Timer &Timer::operator=(Timer &&other)
{
    if (&other == this)
    {
        return *this;
    }

    m_TimestampId = std::move(other.m_TimestampId);
    m_TimerPool = other.m_TimerPool;
    m_CommandBuffer = std::exchange(other.m_CommandBuffer, VK_NULL_HANDLE);
    return *this;
}

Timer::~Timer()
{
    if (m_CommandBuffer != VK_NULL_HANDLE)
    {
        // Should not use anything else than bottom of pipe bit, results within passes
        // are unreliable.
        vkCmdWriteTimestamp(m_CommandBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimerPool.get().GetQueryPool(), m_TimestampId.End);
    }
}
