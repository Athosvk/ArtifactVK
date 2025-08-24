#pragma once
#include <vulkan/Vulkan.h>
#include <unordered_map>
#include <chrono>
#include <string>

#include <backend/Timer.h>
#include <backend/Buffer.h>

struct ResolvedTimerPool
{
    std::unordered_map<std::string, std::chrono::nanoseconds> Timings;
};

// TODO: Embed this as part of a CommandBuffer directly, as that knows when the
// command buffer has executed fully and therefore written its timestamps (i.e. 
// whehter the fence has been signaled).
class TimerPool
{
  public:
    TimerPool(VkDevice device, const PhysicalDevice& physicalDevice);

    TimerPool(const TimerPool &) = delete;
    TimerPool(TimerPool && other);

    TimerPool &operator=(const TimerPool&) = delete;
    TimerPool &operator=(TimerPool && other);

    ~TimerPool();

    Timer BeginScope(VkCommandBuffer commandBuffer, std::string name);

    void Reset(VkCommandBuffer commandBuffer);
    VkQueryPool GetQueryPool() const;
    ResolvedTimerPool Resolve();
  private:
    uint32_t m_Counter = 0;
    VkDevice m_Device;
    VkQueryPool m_QueryPool;
    // Index is the beginning of the timestamp
    std::vector<std::string> m_TimestampIdNames;
    // For caching purposes. Allows not re-allocating every frame
    std::vector<uint64_t> m_ResultBuffer;
    uint64_t m_TimerResolution;
};
