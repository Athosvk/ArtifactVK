#pragma once
#include <vulkan/Vulkan.h>
#include <unordered_map>
#include <chrono>
#include <string>

#include <backend/Timer.h>

class ResolvedTimerPool
{
  public:
  private:
    std::unordered_map<std::string, std::chrono::nanoseconds> m_Timings;
};

class TimerPool
{
  public:
    TimerPool(VkDevice device, uint32_t size);

    TimerPool(const TimerPool &) = delete;
    TimerPool(TimerPool && other);

    TimerPool &operator=(const TimerPool&) = delete;
    TimerPool &operator=(TimerPool && other);

    ~TimerPool();

    Timer BeginScope(VkCommandBuffer commandBuffer);

    VkQueryPool GetQueryPool() const;

    ResolvedTimerPool Resolve() &&;
  private:
    uint32_t m_Counter = 0;
    VkDevice m_Device;
    VkQueryPool m_QueryPool;
    std::unordered_map<std::string, TimestampId> m_LiveTimers;
};
