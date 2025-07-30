#pragma once
#include <vulkan/vulkan.h>

struct Viewport
{
    float AspectRatio() const;

    VkViewport Viewport;
    VkRect2D Scissor;
};


