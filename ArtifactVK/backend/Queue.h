#pragma once
#include <vulkan/vulkan.h>

class Queue
{
  public:
    Queue(VkDevice device, uint32_t queueIndex);

    VkQueue Get() const;
    void Wait() const;
    uint32_t GetFamilyIndex() const;
  private:
    VkQueue m_Queue;
    uint32_t m_QueueFamilyIndex;
};
