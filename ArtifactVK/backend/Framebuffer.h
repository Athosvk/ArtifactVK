#pragma once
#include <vulkan/vulkan.h>

#include "Viewport.h"

class RenderPass;

struct FramebufferCreateInfo
{
    const RenderPass &RenderPass;
    VkImageView ImageView;
    Viewport Viewport;
};

class Framebuffer
{
  public:
    Framebuffer(VkDevice device, const FramebufferCreateInfo& framebufferCreateInfo);
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer(Framebuffer &&other);
    ~Framebuffer();

    VkFramebuffer Get() const;
    Viewport GetViewport() const;
  private:
    VkFramebuffer m_Framebuffer;
    VkDevice m_Device;
    FramebufferCreateInfo m_OriginalCreateInfo;
};
