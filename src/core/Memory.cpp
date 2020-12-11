#include "Memory.h"

uint32_t Djinn::findMemoryType(Instance* p_instance, const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(p_instance->physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		bool memDetect{ (typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) };
		if (memDetect)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");

	return 0;
}