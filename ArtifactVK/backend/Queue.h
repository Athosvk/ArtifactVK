#pragma once
#include <vulkan/vulkan.h>

class Queue
{
  public:
    Queue(VkDevice device, uint32_t queueIndex);

    VkQueue Get() const;
    void Wait() const;
  private:
    VkQueue m_Queue;
};
