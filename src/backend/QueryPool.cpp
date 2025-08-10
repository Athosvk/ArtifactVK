#include <backend/QueryPool.h>

#include <stdexcept>

QueryPool::QueryPool(VkDevice device, uint32_t size, VkQueryType type) : m_Device(device)
{
    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = 4096u;
    if (vkCreateQueryPool(device, &query_pool_info, nullptr, &m_QueryPool) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create query pool");
    }
}

QueryPool::QueryPool(QueryPool && other)
{
    *this = std::move(other);
}

QueryPool &QueryPool::operator=(QueryPool && other)
{
    if (this != &other)
    {
        m_QueryPool = std::move(other.m_QueryPool);

    }
}

QueryPool::~QueryPool()
{
    vkDestroyQueryPool(m_Device, m_QueryPool);
}
