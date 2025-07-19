#include "ExtensionFunctionMapping.h"

ExtensionFunctionMapping::ExtensionFunctionMapping(const VkInstance &vkInstance)
    : m_VkInstance(vkInstance), m_ExtensionFunctionMapping(CreateFunctionMapping(CreateFunctionNameMapping()))
{
}

std::unordered_map<EExtensionFunction, const char *> ExtensionFunctionMapping::CreateFunctionNameMapping() const
{
    std::unordered_map<EExtensionFunction, const char *> nameMapping;
    nameMapping.insert({EExtensionFunction::CreateDebugUtilsMessenger, "vkCreateDebugUtilsMessengerEXT"});
    nameMapping.insert({EExtensionFunction::DestroyDebugUtilsMessenger, "vkDestroyDebugUtilsMessengerEXT"});
    nameMapping.insert({EExtensionFunction::DebugUtilsSetObjectName, "vkSetDebugUtilsObjectNameEXT"});
    return nameMapping;
}

std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> ExtensionFunctionMapping::CreateFunctionMapping(
    const std::unordered_map<EExtensionFunction, const char *> &extensionNameMapping) const
{
    std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> functionMapping;
    functionMapping.reserve(extensionNameMapping.size());
    for (const auto &[function, name] : extensionNameMapping)
    {
        auto extension = vkGetInstanceProcAddr(m_VkInstance, name);
        if (extension != nullptr)
        {
            functionMapping[function] = extension;
        }
    }
    return functionMapping;
}

PFN_vkVoidFunction ExtensionFunctionMapping::GetFunction(EExtensionFunction function) const
{
    // TODO: Graceful hndling of extensions that aren't available
    return m_ExtensionFunctionMapping.at(function);
}
