#include <backend/VulkanDebugMessenger.h>

#include <iostream>
#include <vulkan/vulkan.h>

#include <backend/ExtensionFunctionMapping.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                                                    void *userData)
{
    std::string severity;
    switch (messageType)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severity = "TRACE";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severity = "INFO";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity = "WARNING";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity = "ERROR";
        break;
    }
    if (messageType <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        return VK_FALSE;
    }

    std::cout << severity << ": Validation layer: " << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

VulkanDebugMessenger::VulkanDebugMessenger(VkInstance &vulkanInstance, const ExtensionFunctionMapping &extensionMapper)
    : m_VulkanInstance(vulkanInstance), m_ExtensionMapper(extensionMapper),
      m_DebugMessenger(Create(m_VulkanInstance, m_ExtensionMapper))
{
}

VulkanDebugMessenger::~VulkanDebugMessenger()
{
    if (m_DebugMessenger != VK_NULL_HANDLE)
    {
        auto destroyDebugManager = (PFN_vkDestroyDebugUtilsMessengerEXT)m_ExtensionMapper.GetFunction(
            EExtensionFunction::DestroyDebugUtilsMessenger);
        destroyDebugManager(m_VulkanInstance, m_DebugMessenger, nullptr);
    }
}

VulkanDebugMessenger::VulkanDebugMessenger(VulkanDebugMessenger &&other)
    : m_VulkanInstance(other.m_VulkanInstance), m_DebugMessenger(std::exchange(other.m_DebugMessenger, VK_NULL_HANDLE)),
      m_ExtensionMapper(other.m_ExtensionMapper)
{
}

VkDebugUtilsMessengerCreateInfoEXT VulkanDebugMessenger::CreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    return createInfo;
}

VkDebugUtilsMessengerEXT VulkanDebugMessenger::Create(VkInstance &vulkanInstance,
                                                      const ExtensionFunctionMapping &extensionMapper)
{
    auto createFunction = (PFN_vkCreateDebugUtilsMessengerEXT)extensionMapper.GetFunction(
        EExtensionFunction::CreateDebugUtilsMessenger);
    VkDebugUtilsMessengerEXT debugMessenger;
    auto createInfo = CreateInfo();
    // TODO: Handle errors here
    createFunction(vulkanInstance, &createInfo, nullptr, &debugMessenger);
    return debugMessenger;
}
