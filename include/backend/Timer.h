#pragma once
#include <vulkan/vulkan.h>
#include <functional>

class TimerPool;

struct TimestampId 
{
    uint32_t Begin;
    uint32_t End;
};

class Timer
{
  public:
    Timer(VkCommandBuffer commandBuffer, TimerPool &queryPool, TimestampId timestampId);

    Timer(const Timer &) = delete;
    Timer(Timer &&other);

    Timer &operator=(const Timer &other) = delete;
    Timer &operator=(Timer &&other);

    ~Timer();
  private:
    VkCommandBuffer m_CommandBuffer;
    std::reference_wrapper<TimerPool> m_TimerPool;
    TimestampId m_TimestampId;
};