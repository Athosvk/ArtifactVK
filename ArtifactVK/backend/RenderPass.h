#pragma once
#include <vulkan/vulkan.h>

struct RenderPassCreateInfo
{
    VkAttachmentDescription attachmentDescription;
    VkImageLayout attachmentLayout;
};

class RenderPass
{
  public:
    RenderPass(VkDevice device, RenderPassCreateInfo&& renderPassCreateInfo);
    ~RenderPass();
    RenderPass(const RenderPass &) = delete; 
    RenderPass(RenderPass && other);
  private:
    VkDevice m_Device;
    VkRenderPass m_RenderPass;
};
