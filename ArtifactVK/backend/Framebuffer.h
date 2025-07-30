#pragma once
#include <vulkan/vulkan.h>

#include "Viewport.h"

class RenderPass;

struct FramebufferCreateInfo
{
    const RenderPass &RenderPass;
    VkImageView ColorOutputView;
    VkImageView DepthAttachmentView;
    Viewport Viewport;
};

class Framebuffer
{
  public:
    Framebuffer(VkDevice device, const FramebufferCreateInfo& framebufferCreateInfo);
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer(Framebuffer &&other);
    ~Framebuffer();

    Framebuffer &operator=(const Framebuffer &other) = delete;
    Framebuffer &operator=(Framebuffer &&other) = default;

    VkFramebuffer Get() const;
    Viewport GetViewport() const;
  private:
    VkFramebuffer m_Framebuffer;
    VkDevice m_Device;
    FramebufferCreateInfo m_OriginalCreateInfo;
};
