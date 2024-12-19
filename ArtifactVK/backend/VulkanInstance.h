#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <cassert>
#include <set>
#include <span>

#include "ExtensionFunctionMapping.h"
#include "VulkanDebugMessenger.h"
#include "DeviceExtensionMapping.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"

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
	KhronosValidation = 0x0,
};

const static std::array<EValidationLayer, 1> AvailableValidationLayers();

struct ValidationLayer
{
public:
	EValidationLayer Layers;
	bool DebugOnly;

	std::vector<const char*> GetLayerNames() const;
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

// Goes against RAII, but needed to handle (un)init order correctly in some cases.
// Alternatively we use optional/unique ptr, but this is a bit more specific.
// This will only allow creation/destruction once
template<typename T>
class ManualScope
{
public:
	ManualScope() = default;
	~ManualScope()
	{
		assert(!m_Inner.has_value() && "Manual scope not manually destroyed. Prefer using a RAII base mechanism instead");
	}
	ManualScope(const ManualScope&) = delete;

	const T& operator*() const
	{
		return m_Inner.value();
	}

	T& operator*()
	{
		return m_Inner.value();
	}

	T* operator->()
	{
		return &m_Inner.value();
	}

	const T* operator->() const
	{
		return &m_Inner.value();
	}

	template<typename... Args>
	void ScopeBegin(Args&&... args) 
	{
		assert(!m_Inner.has_value() && "Scope value recreated");
		m_Inner.emplace(std::forward<Args>(args)...);
	}

	void ScopeEnd()
	{
		assert(m_Inner.has_value() && "Scope value not yet created or scope ended multiple times");
		m_Inner = std::nullopt;
	}
private:
	std::optional<T> m_Inner;
};

class LogicalVulkanDevice
{
public: 
	LogicalVulkanDevice(const VulkanDevice& physicalDevice, const std::vector<const char*>& validationLayers);
	LogicalVulkanDevice(const LogicalVulkanDevice& other) = delete;
	LogicalVulkanDevice(LogicalVulkanDevice&& other) = delete;
	~LogicalVulkanDevice();
private:
	static std::vector<VkDeviceQueueCreateInfo> GetQueueCreateInfos(const VulkanDevice& physicalDevice);

	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
};

class VulkanInstance
{
public:
	VulkanInstance(const InstanceCreateInfo& createInfo, GLFWwindow& window);
	~VulkanInstance();
	VulkanInstance(const VulkanInstance& other) = delete;
	VulkanInstance(VulkanInstance&& other) = default;
private:
	static std::vector<const char*> CheckValidationLayers(const std::vector<ValidationLayer>& validationLayers);
	VkDebugUtilsMessengerEXT CreateDebugMessenger() const;
	VkInstance CreateInstance(const InstanceCreateInfo& createInfo);
	VulkanDevice CreatePhysicalDevice(const VulkanSurface& targetSurface, std::span<const EDeviceExtension> deviceExtensions) const;
	VkDevice CreateLogicalDevice(const VulkanDevice& physicalDevice) const;

	VkInstance m_VkInstance;
	ExtensionFunctionMapping m_ExtensionMapper;
	DeviceExtensionMapping m_DeviceExtensionMapper;
	ManualScope<VulkanDebugMessenger> m_VulkanDebugMessenger;
	ManualScope<VulkanSurface> m_Surface;
	// TODO: Technically does't need to be wrapped in a ManualScope, but
	// has a dependency on m_Surface
	ManualScope<VulkanDevice> m_ActiveDevice;
	ManualScope<LogicalVulkanDevice> m_ActiveLogicalDevice;
	std::vector<const char*> m_ValidationLayers;
};

