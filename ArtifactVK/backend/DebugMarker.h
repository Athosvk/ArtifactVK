#pragma once
#include <vulkan/vulkan.h>

#include <string>

#include "ExtensionFunctionMapping.h"

class DebugMarker
{
  public:
    static void SetName(VkDevice vkDevice, const ExtensionFunctionMapping &extensionMapper, VkCommandBuffer handle, 
        const std::string& name);

  private:
    static VkDebugUtilsObjectNameInfoEXT GetDebugObjectName(const std::string& name);
};
