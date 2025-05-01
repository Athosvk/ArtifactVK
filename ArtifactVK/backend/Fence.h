#pragma once
#include <vulkan/vulkan.h>
class Fence
{
  public:
    explicit Fence(VkDevice device);
    Fence(VkDevice device, bool startSignaled);
    Fence(const Fence &) = delete;
    Fence(Fence&& other);
    
    ~Fence();
    void Wait();
    VkFence Get() const;
    bool QuerySignaled() const;
  private:
    VkFence m_Fence;
    VkDevice m_Device;
};
