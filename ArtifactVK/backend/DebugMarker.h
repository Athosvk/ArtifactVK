#pragma once
#include <vulkan/vulkan.h>

#include <string>

#include "ExtensionFunctionMapping.h"

class DebugMarker
{
  public:
    static void SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkCommandBuffer handle, 
        const std::string& name);
    static void SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkDescriptorSet handle, 
        const std::string& name);
    static void SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkCommandPool handle, 
        const std::string& name);
    static void SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkFence handle, 
        const std::string& name);

  private:
    static void SetNameInternal(const std::string& name, const ExtensionFunctionMapping& mapping, VkDevice device,
        uint64_t handle, VkObjectType handleType);
};
