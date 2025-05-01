#pragma once
#include <vulkan/vulkan.h>


class Fence
{
    enum class FenceStatus
    {
        Unsignaled,
        Signaled,
        Reset,
    };

  public:
    explicit Fence(VkDevice device);
    Fence(VkDevice device, bool startSignaled);
    Fence(const Fence &) = delete;
    Fence(Fence&& other);
    
    ~Fence();
    void Wait();
    VkFence Get() const;
    bool QuerySignaled();
    bool WasReset() const;
  private:
    VkFence m_Fence;
    VkDevice m_Device;
    // For debugigng purposes only
    mutable FenceStatus m_Status;
};
