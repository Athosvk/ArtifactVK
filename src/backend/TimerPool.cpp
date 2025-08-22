
#include <backend/TimerPool.h>

#include <stdexcept>
#include <backend/TimerPool.h>

TimerPool::TimerPool(VkDevice device, uint32_t size) : m_Device(device)
{
    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType = VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType = VkQueryType::VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = 4096u;
    if (vkCreateQueryPool(device, &query_pool_info, nullptr, &m_QueryPool) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create query pool");
    }
}

TimerPool::TimerPool(TimerPool && other)
{
    *this = std::move(other);
}

TimerPool &TimerPool::operator=(TimerPool && other)
{
    if (this != &other)
    {
        m_QueryPool = std::move(other.m_QueryPool);
    }
    return *this;
}

TimerPool::~TimerPool()
{
    vkDestroyQueryPool(m_Device, m_QueryPool, nullptr);
}

Timer TimerPool::BeginScope(VkCommandBuffer commandBuffer)
{
    TimestampId id{m_Counter, m_Counter + 1};
    m_Counter += 2;
    return Timer(commandBuffer, *this, id);
}

VkQueryPool TimerPool::GetQueryPool() const
{
    return m_QueryPool;
}

ResolvedTimerPool TimerPool::Resolve() &&
{
    return ResolvedTimerPool();
}
