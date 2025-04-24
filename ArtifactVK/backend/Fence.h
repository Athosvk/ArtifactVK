#pragma once
#include <vulkan/vulkan.h>
class Fence
{
  public:
    Fence(VkDevice device);
    Fence(VkDevice device, bool startSignaled);
    Fence(const Fence &) = delete;
    Fence(Fence&& other);
    
    ~Fence();
    void Wait();
  private:
    VkFence m_Fence;
    VkDevice m_Device;
};
