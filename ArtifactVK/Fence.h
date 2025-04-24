#pragma once
#include <vulkan/vulkan.h>
class Fence
{
  public:
    Fence(VkDevice device);
    ~Fence();

  private:
    VkFence m_Fence;
    VkDevice m_Device;
};
