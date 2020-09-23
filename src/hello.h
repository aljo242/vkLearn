#ifndef HELLO_H
#define HELLO_H

#include <stdexcept>
#include <cstdlib>
#include <cstdint> // UINT32_MAX
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <assert.h>

#include "ext_inc.h"
#include "QueueFamilies.h"
#include "SwapChainSupportDetails.h"
#include "ShaderLoader.h"

constexpr uint32_t WIDTH{ 800 };
constexpr uint32_t HEIGHT{ 600 };
constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

const std::vector<const char*> validationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#if defined(_DEBUG)
constexpr bool enableValidationlayers{ true };
#else
constexpr bool enableValidationlayers{ false };
#endif


class HelloTriangleApp
{
public:
	HelloTriangleApp();
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	void createInstance();
	bool checkValidationLayerSupport();
	void glfwExtensionCheck();
	void vulkanExtensionCheck();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	std::vector<const char*> getRequiredExtensions();
	// TODO expand this
	uint32_t rateDeviceSuitability(VkPhysicalDevice physicalDev);
	void createSurface();
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDev);
	void pickPhysicalDevice();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling,
		const VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(const VkFormat format);
	VkPhysicalDeviceFeatures  populateDeviceFeatures();
	void createLogicalDevice();
	// Swap Chain Extent is the resolution of the swap chain buffer image
	void createSwapChain();
	void cleanupSwapChain();
	void recreateSwapChain();
	void createSwapChainImages();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createDepthResources();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	VkImageView createImageView(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags);
	VkCommandBuffer beginSingleTimeCommands(VkCommandPool& commandPool);
	void endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkImage image, const VkFormat format, const VkImageLayout oldLayout,
		const VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height);
	void createImage(const uint32_t width, const uint32_t height, const VkFormat format,
		const VkImageTiling tiling, const VkImageUsageFlags flags, 
		const VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();	
	void createDescriptorSets();
	void updateUniformBuffer(const uint32_t imageIndex);
	void createBuffer(const VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory, const VkDeviceSize offset);
	void createDescriptorSetLayout();
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, const VkDeviceSize size);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createCommandBuffers();
	void createSyncObjects();
	void drawFrame();

	// matches signature from vulkan api
	static VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApp::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagBitsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			spdlog::debug("Validation Layer {} Message: {}\n", messageType, pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	static void framebufferResizedCallback(GLFWwindow* window, const int width, const int height)
	{
		auto app{ reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window)) };
		app->framebufferResized = true;
	}

private:
	// window data
	GLFWwindow* window								{ nullptr };

	VkInstance instance								{ VK_NULL_HANDLE };
	VkPhysicalDevice physicalDevice					{ VK_NULL_HANDLE };
	VkDevice device									{ VK_NULL_HANDLE };
	VkSurfaceKHR surface							{ VK_NULL_HANDLE };

	VkQueue graphicsQueue							{ VK_NULL_HANDLE };
	VkQueue presentQueue							{ VK_NULL_HANDLE };
	VkQueue transferQueue							{ VK_NULL_HANDLE }; 

	VkDescriptorSetLayout descriptorSetLayout		{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout					{ VK_NULL_HANDLE };
	VkRenderPass renderPass							{ VK_NULL_HANDLE };
	VkPipeline graphicsPipeline						{ VK_NULL_HANDLE };

	VkSwapchainKHR swapChain						{ VK_NULL_HANDLE };
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool gfxCommandPool					{ VK_NULL_HANDLE };
	VkCommandPool transferCommandPool				{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> commandBuffers;

	// synchronization
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame{ 0 };

	bool framebufferResized{ false };

	VkDebugUtilsMessengerEXT debugMessenger{ VK_NULL_HANDLE };

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	// TODO 
	// combine vertex and index buffer int o a single array
	VkSharingMode sharingMode;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding			= 0;
		bindingDescription.stride			= sizeof(Vertex);
		bindingDescription.inputRate		= VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() 
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding		= 0;
		attributeDescriptions[0].location		= 0;
		attributeDescriptions[0].format			= VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset			= offsetof(Vertex, position);

		attributeDescriptions[1].binding		= 0;
		attributeDescriptions[1].location		= 1;
		attributeDescriptions[1].format			= VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset			= offsetof(Vertex, color);

		attributeDescriptions[2].binding		= 0;
		attributeDescriptions[2].location		= 2;
		attributeDescriptions[2].format			= VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset			= offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};


struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};

#endif 