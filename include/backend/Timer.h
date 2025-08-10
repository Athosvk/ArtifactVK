#include <vulkan/vulkan.h>

class Timer
{
  public:
    Timer(VkDevice device);

  private:
    VkDevice m_Device;
};