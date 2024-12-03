#pragma once
#include <vulkan/vulkan.h>
#include <string>

struct Version
{
	uint16_t Patch;
	uint8_t Minor;
	uint8_t Major;

	const uint32_t ToVulkanVersion() const;
};

struct InstanceCreateInfo
{	
	std::string Name;
	Version AppVersion;
	Version EngineVersion;
};

class VulkanInstance
{
public:
	VulkanInstance(const InstanceCreateInfo& createInfo);
	~VulkanInstance();
private:
	VkInstance m_VKInstance;
};

