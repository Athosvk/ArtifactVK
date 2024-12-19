#include "DeviceExtensionMapping.h"

#include <vulkan/vulkan.h>

template<typename TKey, typename TValue>
std::unordered_map<TValue, TKey> InvertUnorderedMap(const std::unordered_map<TKey, TValue>& original)
{
	std::unordered_map<TValue, TKey> inverted;
	inverted.reserve(original.size());
	for (auto [key, value] : original)
	{
		//inverted.insert(value, key);
		inverted.insert(std::make_pair(value, key));
	}
	return inverted;
}

DeviceExtensionMapping::DeviceExtensionMapping() :
	m_NameMapping(CreateNameMapping()),
	m_ReverseMapping(InvertUnorderedMap(m_NameMapping))
{
}

EDeviceExtension DeviceExtensionMapping::At(std::string_view extensionName) const
{
	auto findIter = m_NameMapping.find(extensionName);
	return findIter != m_NameMapping.end() ? findIter->second : EDeviceExtension::Unknown;
}

const char* DeviceExtensionMapping::ReverseMap(std::span<const EDeviceExtension> extensions) const
{
	return nullptr;
}


std::unordered_map<std::string_view, EDeviceExtension> DeviceExtensionMapping::CreateNameMapping()
{
	return {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, EDeviceExtension::VkSwapchain }
	};
}
