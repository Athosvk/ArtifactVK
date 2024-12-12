#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <cassert>

#include "VulkanExtensionMapper.h"
#include "VulkanDebugMessenger.h"

struct Version
{
	uint16_t Patch;
	uint8_t Minor;
	uint8_t Major;

	const uint32_t ToVulkanVersion() const;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
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
		return m_Inner;
	}

	T& operator*()
	{
		return m_Inner;
	}

	T* operator->()
	{
		return &m_Inner;
	}

	const T* operator->() const
	{
		return &m_Inner;
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

class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice physicalDevice);

	const QueueFamilyIndices& GetQueueFamilies() const;
	bool IsValid() const;
private:
	bool Validate() const;
	QueueFamilyIndices FindQueueFamilies() const;

	VkPhysicalDevice m_PhysicalDevice;
	QueueFamilyIndices m_QueueFamilies;
	bool m_Valid;
};

class VulkanInstance
{
public:
	VulkanInstance(const InstanceCreateInfo& createInfo);
	~VulkanInstance();
private:
	static std::vector<const char*> CheckValidationLayers(const std::vector<ValidationLayer>& validationLayers);
	VkDebugUtilsMessengerEXT CreateDebugMessenger() const;
	static VkInstance CreateInstance(const InstanceCreateInfo& createInfo);
	VulkanDevice CreatePhysicalDevice() const;
	VkDevice CreateLogicalDevice(const VulkanDevice& physicalDevice) const;

	VkInstance m_VkInstance;
	VulkanExtensionMapper m_ExtensionMapper;
	ManualScope<VulkanDebugMessenger> m_VulkanDebugMessenger;
	VulkanDevice m_ActiveDevice;
	VkDevice m_ActiveLogicalDevice;
};

