#include "DebugMarker.h"

#include <string>

void DebugMarker::SetName(VkDevice device, const ExtensionFunctionMapping &extensionMapper, VkCommandBuffer handle,
                          const std::string &name)
{
    auto setNameFn = (PFN_vkSetDebugUtilsObjectNameEXT)extensionMapper.GetFunction(EExtensionFunction::DebugUtilsSetObjectName);

    auto info = GetDebugObjectName(name);
    info.objectType = VkObjectType::VK_OBJECT_TYPE_COMMAND_BUFFER;
    info.objectHandle = reinterpret_cast<uint64_t>(handle);
    setNameFn(device, &info);
}

VkDebugUtilsObjectNameInfoEXT DebugMarker::GetDebugObjectName(const std::string& name)
{
    return VkDebugUtilsObjectNameInfoEXT{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
                                          .pNext = nullptr,
                                          .pObjectName = name.c_str()};
}

