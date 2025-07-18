#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <condition_variable>
#include <immintrin.h>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef NDEBUG
#define NDEBUG 0
#endif // !NDEBUG

std::vector<const char *> ValidationLayer::GetLayerNames() const
{
    std::vector<const char *> names;
    names.reserve(_mm_popcnt_u32((uint32_t)Layers));
    for (EValidationLayer availableLayer : AvailableValidationLayers())
    {
        if (((uint32_t)availableLayer & (uint32_t)Layers) == (uint32_t)availableLayer)
        {
            switch (availableLayer)
            {
            case EValidationLayer::KhronosValidation:
                names.emplace_back("VK_LAYER_KHRONOS_validation\0");
            }
        }
    }
    return names;
}

const uint32_t Version::ToVulkanVersion() const
{
    return VK_MAKE_API_VERSION(0, Major, Minor, Patch);
}

VulkanInstance::VulkanInstance(const InstanceCreateInfo &createInfo, GLFWwindow &window)
    : m_VkInstance(CreateInstance(createInfo)), m_ExtensionMapper(ExtensionFunctionMapping(m_VkInstance))
{
    m_VulkanDebugMessenger.ScopeBegin(m_VkInstance, m_ExtensionMapper);

    m_Surface.ScopeBegin(m_VkInstance, window);
    m_ActivePhysicalDevice.ScopeBegin(CreatePhysicalDevice(*m_Surface, std::span{createInfo.RequiredExtensions}));

    m_ActiveDevice.ScopeBegin(
        m_ActivePhysicalDevice->CreateLogicalDevice(m_ValidationLayers, createInfo.RequiredExtensions, window));
    m_ActiveDevice->CreateSwapchain(window, *m_Surface);
}

VulkanInstance::~VulkanInstance()
{
    if (m_VkInstance != VK_NULL_HANDLE)
    {
        m_VulkanDebugMessenger.ScopeEnd();
        m_ActiveDevice.ScopeEnd();
        m_ActivePhysicalDevice.ScopeEnd();
        m_Surface.ScopeEnd();
        vkDestroyInstance(m_VkInstance, nullptr);
    }
}

VulkanInstance::VulkanInstance(VulkanInstance &&other)
    : m_VkInstance(std::exchange(other.m_VkInstance, VK_NULL_HANDLE)),
      m_ExtensionMapper(std::move(other.m_ExtensionMapper)),
      m_DeviceExtensionMapper(std::move(other.m_DeviceExtensionMapper)),
      m_VulkanDebugMessenger(std::move(other.m_VulkanDebugMessenger)), m_Surface(std::move(other.m_Surface)),
      m_ActivePhysicalDevice(std::move(other.m_ActivePhysicalDevice)), m_ActiveDevice(std::move(other.m_ActiveDevice)),
      m_ValidationLayers(std::move(other.m_ValidationLayers))
{
}

VulkanDevice &VulkanInstance::GetActiveDevice()
{
    return *m_ActiveDevice;
}

std::vector<const char *> VulkanInstance::CheckValidationLayers(const std::vector<ValidationLayer> &validationLayers)
{
    std::vector<const char *> requestedLayers;
    for (const auto &validationLayer : validationLayers)
    {
        if (!validationLayer.DebugOnly || !NDEBUG)
        {
            const std::vector<const char *> layerNames = validationLayer.GetLayerNames();
            requestedLayers.reserve(requestedLayers.size() + layerNames.size());
            requestedLayers.insert(requestedLayers.end(), layerNames.begin(), layerNames.end());
        }
    }

    if (requestedLayers.empty())
    {
        return requestedLayers;
    }

    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(availableLayerCount);
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

    std::unordered_map<std::string_view, bool> foundRequestedDebugLayers;

    for (const auto &requestedLayerName : requestedLayers)
    {
        foundRequestedDebugLayers[std::string_view{requestedLayerName}] = false;
    }

    for (const auto &availableLayer : availableLayers)
    {
        foundRequestedDebugLayers[std::string_view{availableLayer.layerName}] = true;
    }

    std::vector<const char *> layers;
    layers.reserve(foundRequestedDebugLayers.size());
    for (const auto &[layerName, wasFound] : foundRequestedDebugLayers)
    {
        if (!wasFound)
        {
            std::cout << "Missing layer " << layerName;
        }
        else
        {
            layers.emplace_back(layerName.data());
        }
    }
    return requestedLayers;
}

VkInstance VulkanInstance::CreateInstance(const InstanceCreateInfo &createInfo)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = createInfo.Name.c_str();
    appInfo.applicationVersion = createInfo.AppVersion.ToVulkanVersion();
    appInfo.pEngineName = "Artifact";
    appInfo.engineVersion = createInfo.EngineVersion.ToVulkanVersion();
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = nullptr;

    m_ValidationLayers = CheckValidationLayers(createInfo.ValidationLayers);

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> requestedExtensions(glfwExtensionCount);
    for (size_t i = 0; i < glfwExtensionCount; i++)
    {
        requestedExtensions[i] = glfwExtensions[i];
    }
    // Required for things like debug markers
    requestedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    std::cout << "Available extensions: ";
    for (const auto &extension : availableExtensions)
    {
        std::cout << "\t" << extension.extensionName << "\n";
    }

    std::unordered_map<std::string_view, bool> foundRequestedExtensions;
    for (const auto &extension : requestedExtensions)
    {
        foundRequestedExtensions[std::string_view{extension}] = false;
    }

    for (const auto &extension : availableExtensions)
    {
        foundRequestedExtensions[std::string_view{extension.extensionName}] = true;
    }

    bool anyNotFound = false;
    for (const auto &[name, wasFound] : foundRequestedExtensions)
    {
        if (!wasFound)
        {
            anyNotFound = true;
            // TODO: Have GLFW request its extensions instead
            std::cout << "Missing requested extension: " << name << "\n";
        }
    }
    if (anyNotFound)
    {
        throw std::runtime_error("Missing extension");
    }
    std::cout << "Found all extensions to init Vulkan\n";

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size());
    instanceInfo.ppEnabledExtensionNames = requestedExtensions.data();
    instanceInfo.enabledLayerCount = 0;
    if (!m_ValidationLayers.empty())
    {
        instanceInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
        instanceInfo.ppEnabledLayerNames = m_ValidationLayers.data();
        std::cout << "Enabled " << m_ValidationLayers.size() << " validation layers\n";
    }
    auto debugMessengerCreateInfo = VulkanDebugMessenger::CreateInfo();
    // Create a temporary debug messenger for logging Vulkan instance creation issues
    instanceInfo.pNext = &debugMessengerCreateInfo;
    VkInstance vkInstance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vkInstance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create vulkan instance");
    }

    return vkInstance;
}

PhysicalDevice VulkanInstance::CreatePhysicalDevice(const VulkanSurface &targetSurface,
                                                  std::span<const EDeviceExtension> requestedExtensions) const
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_VkInstance, &count, nullptr);

    if (count == 0)
    {
        throw std::runtime_error("No available VK devices found");
    }

    std::vector<VkPhysicalDevice> physicalDevices(count);

    vkEnumeratePhysicalDevices(m_VkInstance, &count, physicalDevices.data());

    std::vector<PhysicalDevice> devices;
    devices.reserve(physicalDevices.size());
    for (auto &physicalDevice : physicalDevices)
    {
        // TODO: Early exit if we find the first suitable one, depending on
        // preference function.
        devices.emplace_back(std::move(physicalDevice), targetSurface, m_DeviceExtensionMapper, requestedExtensions);
    }

    auto firstValid = devices.end();
    uint32_t numValidDevices = 0;
    for (auto iter = devices.begin(); iter != devices.end(); iter++)
    {
        if (iter->IsValid())
        {
            firstValid = iter;
            numValidDevices++;
        }
    }
    if (firstValid == devices.end())
    {
        throw std::runtime_error("No suitable VK devices found");
    }
    std::cout << "Found " << numValidDevices << " suitable physical devices\n";
    return std::move(*firstValid);
}

const std::array<EValidationLayer, 1> AvailableValidationLayers()
{
    return {EValidationLayer::KhronosValidation};
}
