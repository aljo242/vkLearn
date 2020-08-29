#include <stdexcept>
#include <cstdlib>
#include <cstdint> // UINT32_MAX
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>

#include "ext_inc.h"

#include "QueueFamilies.h"
#include "SwapChainSupportDetails.h"
#include "ShaderLoader.h"



constexpr uint32_t WIDTH{ 800 };
constexpr uint32_t HEIGHT{ 600 };
constexpr uint32_t MAX_FRAMES_IN_FLIGHT{2};

const std::vector<const char*> validationLayers {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if defined(_DEBUG)
	constexpr bool enableValidationlayers	{true};
#else
	constexpr bool enableValidationlayers   {false};
#endif


VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	const auto func		{(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")};
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocator)
{
	const auto func		{(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")};
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		// currently choosing 8-bit color with non-linear sRGB curve
		// https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	// TODO sort formats for the "next best"

	// default return 
	return availableFormats[0];
}

// TODO change to probably use another present mode
VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		// allows us to create "triple buffering" schemes
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	// default return
	return VK_PRESENT_MODE_FIFO_KHR;
}




class HelloTriangleApp
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void createInstance()
	{
		if (enableValidationlayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not supported!");
		}

		// basic app description
		VkApplicationInfo appInfo{};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName    = "Hello Triangle";
		appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName         = "Djinn";
		appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_2;

#if defined(_DEBUG)
		glfwExtensionCheck();
		vulkanExtensionCheck();
#endif

		uint32_t glfwExtensionCount	  {0};
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo			= &appInfo;
		createInfo.enabledExtensionCount	= glfwExtensionCount;
		createInfo.ppEnabledExtensionNames	= glfwExtensions;
		createInfo.enabledLayerCount		= 0;
		// validation layers + debug info
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if constexpr (enableValidationlayers)
		{
			createInfo.enabledLayerCount	= static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames	= validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext				= (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount	= 0;
			createInfo.pNext				= nullptr;
		}

		// extension info
		const auto extensions				{getRequiredExtensions()};
		createInfo.enabledExtensionCount	= static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames  = extensions.data();


		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount			{0};
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound		{false};

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

		return true;
	}

	void glfwExtensionCheck()
	{
		uint32_t glfwExtensionCount{ 0 };
		const char** glfwExtensions{ glfwGetRequiredInstanceExtensions(&glfwExtensionCount) };

		spdlog::debug("{} Available GLFW Extensions:", glfwExtensionCount);

		for (size_t i = 0; i < glfwExtensionCount; ++i)
		{
			spdlog::debug("\t {}", glfwExtensions[i]);
		}
	}

	void vulkanExtensionCheck()
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

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
		createInfo.pUserData = nullptr; // optional data		
	}

	void setupDebugMessenger()
	{
		if(!enableValidationlayers) 
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to set up a debug messenger!");
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount		{0};
		const char** glfwExtensions		{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); // begin and end pointers

		if constexpr (enableValidationlayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
		VkDebugUtilsMessageTypeFlagBitsEXT messageType, 
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
		void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			
			spdlog::debug("Validation Layer {} Message: {}", messageType, pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	// TODO expand this
	uint32_t rateDeviceSuitability(VkPhysicalDevice dev)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(dev, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(dev, &deviceFeatures);

		uint32_t score					{0};

		//  huge score boost for discrete GPUs
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		// maximum possible size of textures affect graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// don't care if we can't use geometry shaders
		if (!deviceFeatures.geometryShader)
		{
			return 0;
		}

		bool extensionsSupported		{checkDeviceExtensionSupport(dev)};

		if (!extensionsSupported)
		{
			return 0;
		}

		// check the command queue capabilities of the devices
		QueueFamilyIndices indices		{findQueueFamilies(dev, surface)};
		if (!indices.isComplete())
		{
			return 0;
		}

		bool swapChainAdequate{ false };
		// conditional will always be satisfied due to return statement "if (!extensionsSupported) {return 0;}"
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport	{querySwapChainSupport(dev,	surface)};
			// if we support formats AND present modes, swapchain support is "adequate"
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

			if (!swapChainAdequate)
			{
				return 0;
			}
		}

		return score;
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice dev)
	{
		// TODO
		uint32_t extensionCount		{0};
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount		{0};
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::multimap<uint32_t, VkPhysicalDevice> deviceCandidates;

		for (const auto& dev : devices)
		{
			uint32_t score			{rateDeviceSuitability(dev)};
			deviceCandidates.insert(std::make_pair(score, dev));
		}
		
		// choose the best candidate
		if (deviceCandidates.rbegin()->first > 0)
		{
			physicalDevice = deviceCandidates.rbegin()->second;
		}
		else
		{
			throw std::runtime_error("Failed to find a suitable GPU");
		}

	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices		{findQueueFamilies(physicalDevice, surface)};

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority{ 1.0f };
		for (const auto& queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		VkDeviceCreateInfo createInfo{};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos		= queueCreateInfos.data();
		createInfo.queueCreateInfoCount		= static_cast<uint32_t>(queueCreateInfos.size());;
		createInfo.pEnabledFeatures			= &deviceFeatures;
		createInfo.enabledExtensionCount	= static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames  = deviceExtensions.data();

		if constexpr (enableValidationlayers)
		{
			createInfo.enabledLayerCount	= static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames	= validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount	= 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}
		
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}


	// Swap Chain Extent is the resolution of the swap chain buffer image
	VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width;
			int height;

			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D actualExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport	{querySwapChainSupport(physicalDevice, surface)};

		VkSurfaceFormatKHR surfaceFormat	{chooseSwapChainFormat(swapChainSupport.formats)};
		VkPresentModeKHR presentMode		{chooseSwapChainPresentMode(swapChainSupport.presentModes)};
		VkExtent2D extent					{chooseSwapChainExtent(swapChainSupport.capabilities)};

		uint32_t imageCount			{swapChainSupport.capabilities.minImageCount + 1};

		// if maxImageCount == 0, there is no maximum number of images
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType						= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface						= surface;
		createInfo.minImageCount				= imageCount;
		createInfo.imageFormat					= surfaceFormat.format;
		createInfo.imageColorSpace				= surfaceFormat.colorSpace;
		createInfo.imageExtent					= extent;
		createInfo.imageArrayLayers				= 1; // specify more if doing stereoscopic 3D
		createInfo.imageUsage					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 
		// use VK_IMAGE_USAGE_TRANSFER_DST_BIT if post-processing steps desired

		// TODO REVISIT imageSharingMode 
		QueueFamilyIndices indices			{findQueueFamilies(physicalDevice, surface)};
		uint32_t queueFamilyIndices[]		{indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode			= VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount	= 2;
			createInfo.pQueueFamilyIndices		= queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount	= 0;
			createInfo.pQueueFamilyIndices		= nullptr;
		}

		createInfo.preTransform					= swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha				= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// currently ignoring alpha channel
		createInfo.presentMode					= presentMode;
		createInfo.clipped						= VK_TRUE; // ignored obscured for performance benefit
		createInfo.oldSwapchain					= VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		createSwapChainImages();

		swapChainImageFormat					= surfaceFormat.format;
		swapChainExtent							= extent;
	}

	void cleanupSwapChain()
	{
		for (size_t i = 0; i < swapChainFramebuffers.size(); ++i)
		{
			vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		}

		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < swapChainImageViews.size(); ++i)
		{
			vkDestroyImageView(device, swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);

	}

	void recreateSwapChain()
	{
		int width {0};
		int height {0};
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	void createSwapChainImages()
	{
		uint32_t imageCount			{0};
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}

	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image			= swapChainImages[i];
			createInfo.viewType			= VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format			= swapChainImageFormat;
			// default swizzle mapping
			createInfo.components.r		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a		= VK_COMPONENT_SWIZZLE_IDENTITY;
			// subresourceRange describes the image purpose
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create an image view!");
			}
		}
	}

	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format					= swapChainImageFormat;
		colorAttachment.samples					= VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp					= VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment			= 0;
		colorAttachmentRef.layout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint				= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount			= 1;
		subpass.pColorAttachments				= &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType					= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount			= 1;
		renderPassInfo.pAttachments				= &colorAttachment;
		renderPassInfo.subpassCount				= 1;
		renderPassInfo.pSubpasses				= &subpass;
		renderPassInfo.dependencyCount			= 1;
		renderPassInfo.pDependencies			= &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}


	void createGraphicsPipeline()
	{
		const auto vertShaderCode		{ readBinaryFile("shader/vert.spv") };
		const auto fragShaderCode		{ readBinaryFile("shader/frag.spv") };

		VkShaderModule vertShaderModule {createShaderModule(vertShaderCode)};
		VkShaderModule fragShaderModule {createShaderModule(fragShaderCode)};

		VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
		vertShaderStageCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageCreateInfo.stage			= VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageCreateInfo.module		= vertShaderModule;
		vertShaderStageCreateInfo.pName			= "main";

		VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
		fragShaderStageCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageCreateInfo.stage			= VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageCreateInfo.module		= fragShaderModule;
		fragShaderStageCreateInfo.pName			= "main";

		VkPipelineShaderStageCreateInfo shaderStages[]	{vertShaderStageCreateInfo, fragShaderStageCreateInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
		vertexInputCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
		inputAssemblyCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable		= VK_FALSE;

		VkViewport viewport{};
		viewport.x											= 0.0f;
		viewport.y											= 0.0f;
		viewport.width										= static_cast<float>(swapChainExtent.width);
		viewport.height										= static_cast<float>(swapChainExtent.height);
		viewport.minDepth									= 0.0f;
		viewport.maxDepth									= 1.0f;

		VkRect2D scissor{};
		scissor.offset										= {0, 0};
		scissor.extent										= swapChainExtent;


		VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount				= 1;
		viewportStateCreateInfo.pViewports					= &viewport;
		viewportStateCreateInfo.scissorCount				= 1;
		viewportStateCreateInfo.pScissors					= &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType									= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable							= VK_FALSE;
		rasterizer.rasterizerDiscardEnable					= VK_FALSE;
		rasterizer.polygonMode								= VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth								= 1.0f;
		rasterizer.cullMode									= VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace								= VK_FRONT_FACE_CLOCKWISE;

		// used during shadow mapping
		rasterizer.depthBiasEnable							= VK_FALSE;
		rasterizer.depthBiasConstantFactor					= 0.0f; // Optional
		rasterizer.depthBiasClamp							= 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor						= 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
		multisamplingCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingCreateInfo.sampleShadingEnable			= VK_FALSE;
		multisamplingCreateInfo.rasterizationSamples		= VK_SAMPLE_COUNT_1_BIT;
		multisamplingCreateInfo.minSampleShading			= 1.0f;		// Optional
		multisamplingCreateInfo.pSampleMask					= nullptr;	// Optional
		multisamplingCreateInfo.alphaToCoverageEnable		= VK_FALSE; // Optional
		multisamplingCreateInfo.alphaToOneEnable			= VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask					= VK_COLOR_COMPONENT_R_BIT |
															  VK_COLOR_COMPONENT_G_BIT |
															  VK_COLOR_COMPONENT_B_BIT |
															  VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable					= VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor			= VK_BLEND_FACTOR_ONE;  // Optional
		colorBlendAttachment.dstColorBlendFactor			= VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp					= VK_BLEND_OP_ADD;		// Optional
		colorBlendAttachment.srcAlphaBlendFactor			= VK_BLEND_FACTOR_ONE;  // Optional
		colorBlendAttachment.dstAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp					= VK_BLEND_OP_ADD;		// Optional

		VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
		colorBlendingCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCreateInfo.logicOpEnable				= VK_FALSE;
		colorBlendingCreateInfo.logicOp						= VK_LOGIC_OP_COPY;		// Optional
		colorBlendingCreateInfo.attachmentCount				= 1;
		colorBlendingCreateInfo.pAttachments				= &colorBlendAttachment;
		colorBlendingCreateInfo.blendConstants[0]			= 0.0f;					// Optional
		colorBlendingCreateInfo.blendConstants[1]			= 0.0f;					// Optional
		colorBlendingCreateInfo.blendConstants[2]			= 0.0f;					// Optional
		colorBlendingCreateInfo.blendConstants[3]			= 0.0f;					// Optional

		constexpr uint32_t dynamicStatesSize				{2};
		VkDynamicState dynamicStates[dynamicStatesSize]		{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH};

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount			= dynamicStatesSize;
		dynamicStateCreateInfo.pDynamicStates				= dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount					= 0;					// Optional
		pipelineLayoutInfo.pSetLayouts						= nullptr;				// Optional
		pipelineLayoutInfo.pushConstantRangeCount			= 0;					// Optional
		pipelineLayoutInfo.pPushConstantRanges				= nullptr;				// Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType							= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount						= dynamicStatesSize;
		pipelineCreateInfo.pStages							= shaderStages;
		pipelineCreateInfo.pVertexInputState				= &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState				= &inputAssemblyCreateInfo;
		pipelineCreateInfo.pViewportState					= &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState				= &rasterizer;
		pipelineCreateInfo.pMultisampleState				= &multisamplingCreateInfo;
		pipelineCreateInfo.pDepthStencilState				= nullptr;				// Optional
		pipelineCreateInfo.pColorBlendState					= &colorBlendingCreateInfo; 
		pipelineCreateInfo.pDynamicState					= nullptr;				// Optional
		pipelineCreateInfo.layout							= pipelineLayout;
		pipelineCreateInfo.renderPass						= renderPass;
		pipelineCreateInfo.subpass							= 0;
		pipelineCreateInfo.basePipelineHandle				= VK_NULL_HANDLE;		// Optional
		pipelineCreateInfo.basePipelineIndex				= -1;					// Optional

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	void createFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			VkImageView attachments[]	{swapChainImageViews[i]};

			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType						= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass				= renderPass;
			framebufferCreateInfo.attachmentCount			= 1;
			framebufferCreateInfo.pAttachments				= attachments;
			framebufferCreateInfo.width						= swapChainExtent.width;
			framebufferCreateInfo.height					= swapChainExtent.height;
			framebufferCreateInfo.layers					= 1;

			if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices				{findQueueFamilies(physicalDevice, surface)};

		VkCommandPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType								= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex						= queueFamilyIndices.graphicsFamily.value();
		poolCreateInfo.flags								= 0;	// Optional

		if (vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool!");
		}
	}

	void createCommandBuffers()
	{
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType										= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool								= commandPool;
		allocInfo.level										= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount						= static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers!");
		}


		// begin command buffer recording
		for (size_t i = 0; i < commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType									= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags									= 0;		// Optional
			beginInfo.pInheritanceInfo						= nullptr;  // Optional (use when using secondary command buffers)

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to begin recording command buffers!");
			}


			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType							= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass						= renderPass;
			renderPassInfo.framebuffer						= swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset				= {0, 0};
			renderPassInfo.renderArea.extent				= swapChainExtent;

			VkClearValue clearColor							= {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount					= 1;
			renderPassInfo.pClearValues						= &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Faired to record command buffer!");
			}
		}
	}

	void createSyncObjects()
	{
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType									= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType										= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags										= VK_FENCE_CREATE_SIGNALED_BIT;

		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create sync objects!");
			}
		}
	}

	void drawFrame()
	{
		// wait for fence from previous vkQueueSubmit call
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result {vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex)};

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swapchain image!");
		}

		// check if a previous frame is using this image
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}

		// mark image as "in-use"
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo{};
		submitInfo.sType									= VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[]						{imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[]					{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount						= 1;
		submitInfo.pWaitSemaphores							= waitSemaphores;
		submitInfo.pWaitDstStageMask						= waitStages;
		submitInfo.commandBufferCount						= 1;
		submitInfo.pCommandBuffers							= &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[]						{renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount						= 1;
		submitInfo.pSignalSemaphores						= signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType									= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount						= 1;
		presentInfo.pWaitSemaphores							= signalSemaphores;

		VkSwapchainKHR swapChains[]							{swapChain};
		presentInfo.swapchainCount							= 1;
		presentInfo.pSwapchains								= swapChains;
		presentInfo.pImageIndices							= &imageIndex;
		presentInfo.pResults								= nullptr;		// Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result != VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swapchain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		
	}

	static void framebufferResizedCallback(GLFWwindow* window, const int width, const int height)
	{
		auto app	{reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window))};
		app->framebufferResized = true;
	}


	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// TODO handle this and allow for resizable backbuffers
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
	}

	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop()
	{
		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void cleanup()
	{ 
		cleanupSwapChain();

		// destroy sync objects
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);

		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		if constexpr (enableValidationlayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

private:
	// window data
	GLFWwindow* window								{nullptr};

	VkInstance instance								{ VK_NULL_HANDLE };
	VkPhysicalDevice physicalDevice					{ VK_NULL_HANDLE };		
	VkDevice device									{ VK_NULL_HANDLE };	
	VkSurfaceKHR surface							{ VK_NULL_HANDLE };

	VkQueue graphicsQueue							{ VK_NULL_HANDLE };
	VkQueue presentQueue							{ VK_NULL_HANDLE };

	VkPipelineLayout pipelineLayout					{ VK_NULL_HANDLE };
	VkRenderPass renderPass							{ VK_NULL_HANDLE };
	VkPipeline graphicsPipeline						{ VK_NULL_HANDLE };

	VkSwapchainKHR swapChain						{ VK_NULL_HANDLE };
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool						{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> commandBuffers;

	// synchronization
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
	std::array<VkSemaphore,  MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame								{0};

	bool framebufferResized							{false};

	VkDebugUtilsMessengerEXT debugMessenger			{ VK_NULL_HANDLE };
};


int main()
{
#if defined(_DEBUG)
	spdlog::set_level(spdlog::level::debug);
#endif

	HelloTriangleApp app;

	try 
	{
		app.run();
	}
	catch(const std::exception& e)
	{
		spdlog::error("{}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}