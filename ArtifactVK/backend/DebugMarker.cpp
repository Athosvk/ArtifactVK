#include "DebugMarker.h"

#include <string>
#include <stdexcept>

void DebugMarker::SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkCommandBuffer handle,
                          const std::string &name)
{
    SetNameInternal(name, extensionMapper, device, reinterpret_cast<uint64_t>(handle), VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER);
}

void DebugMarker::SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkDescriptorSet handle,
                          const std::string &name)
{

    SetNameInternal(name, extensionMapper, device, reinterpret_cast<uint64_t>(handle), VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET);
    
}

void DebugMarker::SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkCommandPool handle,
                          const std::string &name)
{
    SetNameInternal(name, extensionMapper, device, reinterpret_cast<uint64_t>(handle), VkObjectType::VK_OBJECT_TYPE_COMMAND_POOL);
}

void DebugMarker::SetNameInternal(const std::string& name, const ExtensionFunctionMapping& mapping, VkDevice device,
    uint64_t handle, VkObjectType handleType)
{
    auto setNameFn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        mapping.GetFunction(EExtensionFunction::DebugUtilsSetObjectName));
    auto info = VkDebugUtilsObjectNameInfoEXT{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                          .pNext = nullptr,
                                         .objectType = handleType,
                                         .objectHandle = handle,
                                         .pObjectName = name.c_str()
    };

    if (setNameFn(device, &info) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not set name");
    }
}

