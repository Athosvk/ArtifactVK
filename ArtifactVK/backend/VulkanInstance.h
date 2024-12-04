#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>

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
};

class VulkanInstance
{
public:
	VulkanInstance(const InstanceCreateInfo& createInfo);
	~VulkanInstance();
private:
	std::vector<const char*> CheckValidationLayers(const std::vector<ValidationLayer>& validationLayers) const;

	VkInstance m_VKInstance;
};

