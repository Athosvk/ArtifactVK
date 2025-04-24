#pragma once
#include <vulkan/vulkan.h>

class Semaphore
{
  public:
    Semaphore(VkDevice device);
    ~Semaphore();

  private:
    VkSemaphore m_Semaphore;
    VkDevice m_Device;
};
