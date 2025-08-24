#include <backend/TimerPool.h>

#include <stdexcept>

#include <backend/TimerPool.h>
#include <backend/PhysicalDevice.h>

constexpr uint16_t QueryCount = 4096;

TimerPool::TimerPool(VkDevice device, const PhysicalDevice &physicalDevice)
    : m_Device(device), m_TimerResolution(physicalDevice.GetProperties().limits.timestampPeriod)
{
    m_ResultBuffer.resize(QueryCount);
    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType = VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType = VkQueryType::VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = QueryCount;
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

Timer TimerPool::BeginScope(VkCommandBuffer commandBuffer, std::string name)
{
    TimestampId id{m_Counter, m_Counter + 1};
    m_TimestampIdNames.push_back(name);
    m_Counter += 2;
    return Timer(commandBuffer, *this, id);
}

VkQueryPool TimerPool::GetQueryPool() const
{
    return m_QueryPool;
}

void TimerPool::Reset(VkCommandBuffer commandBuffer)
{
    if (!m_TimestampIdNames.empty())
    {
        throw std::runtime_error("Resetting TimerPool with unresolved timings");
    }
    vkCmdResetQueryPool(commandBuffer, m_QueryPool, 0, QueryCount);
}

ResolvedTimerPool TimerPool::Resolve()
{
    ResolvedTimerPool resolvedTimings;
    if (m_Counter)
    {
        // TODO: Check if all timers were actually "ended", so that we don't try
        // to resolve "end" timestamps that were actually never written as cmd to
        // the command buffer.
        vkGetQueryPoolResults(
            m_Device, m_QueryPool, 0, m_Counter, sizeof(uint64_t) * m_Counter, m_ResultBuffer.data(), sizeof(uint64_t),
            VkQueryResultFlagBits::VK_QUERY_RESULT_64_BIT | VkQueryResultFlagBits::VK_QUERY_RESULT_WAIT_BIT);

        auto timestampIdNames = std::move(m_TimestampIdNames);
        for (int i = 0; i < m_Counter; i += 2)
        {
            // Divide by 2, since counter advances by 2 for every
            // every timestamp for a begin and end timestamp.
            std::string name = timestampIdNames[i / 2];
            uint64_t nanos = (m_ResultBuffer[i + 1] - m_ResultBuffer[i]) * m_TimerResolution;
            resolvedTimings.Timings[timestampIdNames[i / 2]] += std::chrono::nanoseconds(nanos);
        }

        m_Counter = 0;
    }
    // TODO: Automatically invoke Reset here?
    return resolvedTimings;
}
