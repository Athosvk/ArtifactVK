#pragma once
#include <vulkan/vulkan.h>

struct RenderPassCreateInfo
{
    // Abstract to attachment, encode with width/height
    VkAttachmentDescription swapchainAttachmentDescription;
};

class RenderPass
{
  public:
    RenderPass(VkDevice device, RenderPassCreateInfo&& renderPassCreateInfo);
    ~RenderPass();
    RenderPass(const RenderPass &) = delete; 
    RenderPass(RenderPass && other);

    VkRenderPass Get() const;
  private:
    VkDevice m_Device;
    VkRenderPass m_RenderPass;
};
