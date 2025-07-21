#pragma once
#include <vulkan/vulkan.h>

#include <functional>

class DepthAttachment;

struct RenderPassCreateInfo
{
    // TODO: Abstract to attachment, encode with width/height
    VkAttachmentDescription SwapchainAttachmentDescription;
    DepthAttachment& DepthAttachment;
};

class RenderPass
{
  public:
    RenderPass(VkDevice device, RenderPassCreateInfo&& renderPassCreateInfo);
    ~RenderPass();
    RenderPass(const RenderPass &) = delete; 
    RenderPass(RenderPass && other);

    VkRenderPass Get() const;
    VkImageView GetDepthAttachmentView() const;
  private:
    VkDevice m_Device;
    VkRenderPass m_RenderPass;
    std::reference_wrapper<DepthAttachment> m_DepthAttachment;
};
