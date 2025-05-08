#pragma once
#include <vulkan/vulkan.h>

class Semaphore
{
  public:
    Semaphore(VkDevice device);
    Semaphore(const Semaphore &) = delete;
    Semaphore(Semaphore && other);
    ~Semaphore();

    VkSemaphore Get() const;

  private:
    VkSemaphore m_Semaphore;
    VkDevice m_Device;
};
