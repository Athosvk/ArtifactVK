#pragma once
#include <array>
#include <cassert>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include "DeviceExtensionMapping.h"
#include "ExtensionFunctionMapping.h"
#include "VulkanDebugMessenger.h"
#include "VulkanDevice.h"
#include "PhysicalDevice.h"
#include "VulkanSurface.h"
#include "../ManualScope.h"

struct GLFWwindow;

struct Version
{
    uint16_t Patch;
    uint8_t Minor;
    uint8_t Major;

    const uint32_t ToVulkanVersion() const;
};

enum class EValidationLayer : uint32_t
{
    KhronosValidation = 0x1,
};

const static std::array<EValidationLayer, 1> AvailableValidationLayers();

struct ValidationLayer
{
  public:
    EValidationLayer Layers;
    bool DebugOnly;

    std::vector<const char *> GetLayerNames() const;
};

struct InstanceCreateInfo
{
    std::string Name;
    Version AppVersion;
    Version EngineVersion;
    std::vector<ValidationLayer> ValidationLayers;
    std::vector<EDeviceExtension> RequiredExtensions;
    std::vector<EDeviceExtension> OptionalExtensions;
};

class VulkanInstance
{
  public:
    VulkanInstance(const InstanceCreateInfo &createInfo, GLFWwindow &window);
    ~VulkanInstance();
    VulkanInstance(const VulkanInstance &other) = delete;
    VulkanInstance(VulkanInstance &&other);

    VulkanDevice &GetActiveDevice();
    const ExtensionFunctionMapping &GetExtensionFunctionMapping() const; 
  private:
    static std::vector<const char *> CheckValidationLayers(const std::vector<ValidationLayer> &validationLayers);
    VkDebugUtilsMessengerEXT CreateDebugMessenger() const;
    VkInstance CreateInstance(const InstanceCreateInfo &createInfo);
    PhysicalDevice CreatePhysicalDevice(const VulkanSurface &targetSurface,
                                      std::span<const EDeviceExtension> deviceExtensions) const;

    // Ugly hack to just store the validation layers here
    // TODO: Don't have this be init order dependent
    std::vector<const char *> m_ValidationLayers;
    VkInstance m_VkInstance;
    ExtensionFunctionMapping m_ExtensionMapper;
    DeviceExtensionMapping m_DeviceExtensionMapper;
    ManualScope<VulkanDebugMessenger> m_VulkanDebugMessenger;
    ManualScope<VulkanSurface> m_Surface;
    // TODO: Technically does't need to be wrapped in a ManualScope, but
    // has a dependency on m_Surface
    ManualScope<PhysicalDevice> m_ActivePhysicalDevice;
    ManualScope<VulkanDevice> m_ActiveDevice;
};
