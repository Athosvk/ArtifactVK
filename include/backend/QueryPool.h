#include <vulkan/Vulkan.h>

class QueryPool
{
    QueryPool(VkDevice device, uint32_t size, VkQueryType type);
    QueryPool(const QueryPool &) = delete;
    QueryPool(QueryPool && other);

    QueryPool &operator=(const QueryPool&) = delete;
    QueryPool &operator=(QueryPool && other);

    ~QueryPool();
  private:
    VkDevice m_Device;
    VkQueryPool m_QueryPool;
};
