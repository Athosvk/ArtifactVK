#pragma once
#include <vulkan/vulkan.h>

struct Viewport
{
    VkViewport Viewport;
    VkRect2D Scissor;
};
