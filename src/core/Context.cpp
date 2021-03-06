#include <map>
#include <string>
#include <set>

#include "Context.h"
#include "SwapChain.h"
#include "defs.h"
#include "../gfxDebug.h"
#include "../QueueFamilies.h"

void Djinn::Context::Init()
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INIT WINDOW
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan Window", nullptr, nullptr);

	// use this to get access members in the resize callback fxn
	// window now will point to class, so it can access private members
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetCursorPosCallback(window, cursorPositionCallback);





	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not supported!");
	}

	// basic app description
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = APP_NAME.c_str();
	appInfo.applicationVersion = APP_VERSION;
	appInfo.pEngineName = ENGINE_NAME.c_str();
	appInfo.engineVersion = ENGINE_VERSION;

	if (vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion") == NULL)
	{
		throw std::runtime_error("No Vulkan instance found.");
	}

	// check for the correct API version
	// Djinn currently will only support Vulkan spec 1.1 and higher
	uint32_t vulkanVersion{ 0 };
	vkEnumerateInstanceVersion(&vulkanVersion);
	if (vulkanVersion < VK_API_VERSION_1_1)
	{
		throw std::runtime_error("Device only supports Vulkan 1.0.  Djinn currently supports Vulkan 1.1 and 1.2");
	}
	appInfo.apiVersion = vulkanVersion;

#if defined(_DEBUG)
	glfwExtensionCheck();
	vulkanExtensionCheck();
#endif

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	// debug info
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if constexpr (ENABLE_VALIDATION_LAYERS)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.pNext = nullptr;
	}

	// extension info
	const auto extensions{ getRequiredExtensions() };
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// CREATE INSTANCE
	auto result{ vkCreateInstance(&instanceCreateInfo, nullptr, &instance) };
	DJINN_VK_ASSERT(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// SETUP DEBUG MESSENGER
	if constexpr (ENABLE_VALIDATION_LAYERS)
	{
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
		populateDebugMessengerCreateInfo(messengerCreateInfo);

		result = CreateDebugUtilsMessengerEXT(instance, &messengerCreateInfo, nullptr, &debugMessenger.handle);
		DJINN_VK_ASSERT(result);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// CREATE SURFACE
	// exposing vulkan functions of glfw, so window creation becomes simplified
	result = (glfwCreateWindowSurface(instance, window, nullptr, &surface));
	DJINN_VK_ASSERT(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// SET UP PHYSICAL DEVICE
	uint32_t deviceCount{ 0 };
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	// <score, device number>
	std::multimap<uint32_t, VkPhysicalDevice> deviceCandidates;

	for (const auto& dev : physicalDevices)
	{
		uint32_t score{ rateDeviceSuitability(dev) };
		deviceCandidates.insert(std::make_pair(score, dev));
	}

	// choose the best candidate
	// rbegin will choose the highest score
	// reject if there are none greater than 0 as rateDeviceSuitability 
	// only returns 0 if the device is NOT suitable
	if (deviceCandidates.rbegin()->first > 0)
	{
		gpuInfo.gpu = deviceCandidates.rbegin()->second;
		vkGetPhysicalDeviceMemoryProperties(gpuInfo.gpu, &gpuInfo.memProperties);
		vkGetPhysicalDeviceProperties(gpuInfo.gpu, &gpuInfo.gpuProperties);
		renderConfig.msaaSamples = getMaxUsableSampleCount();
	}
	else
	{
		throw std::runtime_error("Failed to find a suitable GPU");
	}

	// cache properties for later queries


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// SET UP LOGICAL DEVICE

	queueFamilyIndices =  findQueueFamilies(gpuInfo.gpu, surface);

	// if both queues have the same indices
	std::set<uint32_t> uniqueQueueFamilies{ queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.transferFamily.value() };
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());

	constexpr float queuePriority{ 1.0f };
	size_t i{ 0 };
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[i] = queueCreateInfo;
		++i;
	}

	VkPhysicalDeviceFeatures deviceFeatures{ populateDeviceFeatures() };
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	// DEPRECATED
	// consider removing
	if constexpr (ENABLE_VALIDATION_LAYERS)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	result = vkCreateDevice(gpuInfo.gpu, &deviceCreateInfo, nullptr, &gpuInfo.device);
	DJINN_VK_ASSERT(result);

	// GET QUEUES
	vkGetDeviceQueue(gpuInfo.device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(gpuInfo.device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	vkGetDeviceQueue(gpuInfo.device, queueFamilyIndices.transferFamily.value(), 0, &transferQueue);

	createCommandPools();
}


void Djinn::Context::queryWindowSize()
{
	int width{ 0 };
	int height{ 0 };
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	// minimized window -> do no work
	while (windowWidth == 0 || windowHeight == 0)
	{
		glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
		glfwWaitEvents();
	}
}


void Djinn::Context::CleanUp()
{
	vkDestroyCommandPool(gpuInfo.device, graphicsCommandPool, nullptr);
	vkDestroyCommandPool(gpuInfo.device, transferCommandPool, nullptr);

	vkDestroyDevice(gpuInfo.device, nullptr);

	if constexpr (ENABLE_VALIDATION_LAYERS)
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger.handle, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}


bool Djinn::Context::checkValidationLayerSupport()
{
	uint32_t layerCount{ 0 };
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VALIDATION_LAYERS)
	{
		bool layerFound{ false };
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}
}


void Djinn::Context::glfwExtensionCheck()
{
	uint32_t glfwExtensionCount{ 0 };
	const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

	spdlog::debug("{} Available GLFW Extensions:", glfwExtensionCount);

	for (size_t i = 0; i < glfwExtensionCount; ++i)
	{
		spdlog::debug("\t {}", glfwExtensions[i]);
	}
}

void Djinn::Context::vulkanExtensionCheck()
{
	uint32_t extensionCount{ 0 };
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	spdlog::debug("{} Available Vulkan Extensions:", extensionCount);

	for (const auto& extension : extensions)
	{
		spdlog::debug("\t {}", extension.extensionName);
	}
}


void Djinn::Context::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
#if defined(_DEBUG)
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugMessenger.debugCallback;
	createInfo.pUserData = &debugMessenger;				// optional data
#endif
}

std::vector<const char*> Djinn::Context::getRequiredExtensions()
{
	uint32_t glfwExtensionCount{ 0 };
	const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

	if (glfwExtensionCount == 0)
	{
		throw std::runtime_error("Error querying GLFW extension support.");
	}

	// copy array into vector object for ease of use
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); // begin and end pointers

	if constexpr (ENABLE_VALIDATION_LAYERS)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


// TODO expand this
uint32_t Djinn::Context::rateDeviceSuitability(VkPhysicalDevice physicalDev)
{
	VkPhysicalDeviceProperties2 deviceProperties;
	deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	// TODO use pNext to query specific necessary features
	deviceProperties.pNext = nullptr;
	vkGetPhysicalDeviceProperties2(physicalDev, &deviceProperties);

	VkPhysicalDeviceFeatures2 deviceFeatures;
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = nullptr;
	vkGetPhysicalDeviceFeatures2(physicalDev, &deviceFeatures);

	uint32_t score{ 0 };

	//  huge score boost for discrete GPUs
	if (deviceProperties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_OTHER)
	{
		score += 1000 * (4 - deviceProperties.properties.deviceType);
	}

	// prefer device with 1.2 over 1.1
	if (deviceProperties.properties.apiVersion >= VK_API_VERSION_1_2)
	{
		score += 500;
	}

	// maximum possible size of textures affect graphics quality
	score += deviceProperties.properties.limits.maxImageDimension2D;

	// don't care if we can't use geometry shaders
	// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDeviceFeatures.html
	// if engine requires any of these features, may be required like shown 
	// below with geometry shader requirements
	if (!deviceFeatures.features.geometryShader)
	{
		return 0;
	}

	if (!checkDeviceExtensionSupport(physicalDev))
	{
		return 0;
	}

	// check if we support graphics queues and presentation
	auto indices{ findQueueFamilies(physicalDev, surface) };
	if (!indices.isComplete())
	{
		return 0;
	}

	// prefer to do graphics and presentation on the same device
	if (indices.graphicsFamily.value() == indices.presentFamily.value())
	{
		score += 1000;
	}

	const auto swapChainSupport{ querySwapChainSupport(physicalDev,	surface) };
	// if we support formats AND present modes, swapchain support is "adequate"
	bool swapChainAdequate{ !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty() };
	if (!swapChainAdequate)
	{
		return 0;
	}

	return score;
}

VkSampleCountFlagBits Djinn::Context::getMaxUsableSampleCount()
{
	const VkSampleCountFlags countFlags = gpuInfo.gpuProperties.limits.framebufferColorSampleCounts &
		gpuInfo.gpuProperties.limits.framebufferDepthSampleCounts;

	if (countFlags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	else if (countFlags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	else if (countFlags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	else if (countFlags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	else if (countFlags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	else if (countFlags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
	else
	{
 		return VK_SAMPLE_COUNT_1_BIT;
	}
}

// build up a set up of required extensions
// check if required extensions is a subset of available extensions
// report if true or false
bool Djinn::Context::checkDeviceExtensionSupport(VkPhysicalDevice physicalDev)
{
	uint32_t extensionCount{ 0 };
	vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDev, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkPhysicalDeviceFeatures Djinn::Context::populateDeviceFeatures()
{
	VkPhysicalDeviceFeatures deviceFeatures{};

	// TODO 
	// add all needed features
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;

	// multi sampling features
	deviceFeatures.variableMultisampleRate = VK_TRUE;
	deviceFeatures.alphaToOne = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	// anisotropic filtering
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	// enable wireframe
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	return deviceFeatures;
}



void Djinn::Context::createCommandPools()
{
	const auto graphicsFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	const auto transferFamilyIndex = queueFamilyIndices.transferFamily.value();

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	auto result{ (vkCreateCommandPool(gpuInfo.device, &poolCreateInfo, nullptr, &graphicsCommandPool)) };
	DJINN_VK_ASSERT(result);

	// transfer pool
	poolCreateInfo.queueFamilyIndex = transferFamilyIndex;

	// transfer commands are short-lived, so this hint could lead to allocation optimizations
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	result = (vkCreateCommandPool(gpuInfo.device, &poolCreateInfo, nullptr, &transferCommandPool));
	DJINN_VK_ASSERT(result);
}