#ifndef DJINN_VULKAN_ENGINE_INCLUDE_H
#define DJINN_VULKAN_ENGINE_INCLUDE_H

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
#include "ShaderLoader.h"
#include "core/core.h"
#include "core/Image.h"
#include "core/Buffer.h"
#include "core/Primitives.h"
#include "core/GraphicsPipeline.h"
#include "core/RenderPass.h"
#include <vulkan/vulkan.h>
#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_vulkan.h"
#include "external/imgui/backends/imgui_impl_glfw.h"
#include "external/vk_mem_alloc.h"

#include "DjinnLib/Array.h"
#include "DjinnLib/Queue.h"


constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

namespace Djinn
{
	class Context;
	class SwapChain;


	class VulkanEngine
	{
	public:
		VulkanEngine() = default;
		~VulkanEngine();
		void Init();
		bool WindowOpen();
		void QueryWindowEvents();
		void drawFrame();
		void CleanUp();
		Djinn::KeyboardState* GetKeyboardState() const;
		Djinn::MouseState* GetMouseState() const;
		Djinn::GamepadState* GetGamepadState() const;


	private:
		void initVulkan();

		// Swap Chain Extent is the resolution of the swap chain buffer image
		void cleanupSwapChain();
		void recreateSwapChain();
		void createRenderPass();
		void createGraphicsPipeline();
		void createDepthResources();
		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();
		void createColorResources();
		VkImageView createImageView(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t mipLevels);
		VkCommandBuffer beginSingleTimeCommands(VkCommandPool& commandPool);
		void endSingleTimeCommands(VkCommandPool& commandPool, VkCommandBuffer commandBuffer, VkQueue submitQueue);
		void transitionImageLayout(VkImage image, const VkFormat format, const VkImageLayout oldLayout,
			const VkImageLayout newLayout, const uint32_t mipLevels);
		void copyBufferToImage(VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height);
		void generateMipMaps(VkImage image, const VkFormat format, const uint32_t texWidth, const uint32_t texHeight, const uint32_t mipLevels);
		void createImage(const uint32_t width, const uint32_t height, const uint32_t mipLevels, const VkFormat format,
			const VkSampleCountFlagBits numSamples, const VkImageTiling tiling, const VkImageUsageFlags flags,
			const VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		void loadModel(const std::string& path);
		void createVertexBufferStaged();
		void createIndexBufferStaged();
		void createUniformBuffers();
		void createDescriptorPool();
		void createDescriptorSets();
		void updateUniformBuffer(const uint32_t imageIndex);
		void createBuffer(const VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
			VkBuffer& buffer, VkDeviceMemory& bufferMemory, const VkDeviceSize offset);
		void createDescriptorSetLayout();
		void createCommandBuffers();
		void createSyncObjects();
		void initImGui();
		void initVMA();

	private:

		Djinn::Context* p_context{ nullptr };
		Djinn::SwapChain* p_swapChain{ nullptr };
		Djinn::Queue mainDeletionQueue;
		Djinn::Queue swapchainDeletionQueue;
		VmaAllocator VMA;

		ImGui_ImplVulkanH_Window g_MainWindowData;

		VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
		Djinn::GraphicsPipeline graphicsPipeline;
		Djinn::RenderPass renderPass;

		//VkCommandPool gfxCommandPool					{ VK_NULL_HANDLE };
		//VkCommandPool transferCommandPool				{ VK_NULL_HANDLE };
		std::vector<VkCommandBuffer> commandBuffers;

		// synchronization
		Djinn::Array1D<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
		Djinn::Array1D<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
		Djinn::Array1D<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;

		std::vector<VkFence> imagesInFlight;
		size_t currentFrame{ 0 };

		bool framebufferResized{ false };

		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;

		// TODO 
		// combine vertex and index buffer int o a single array
		Djinn::Buffer _vertexBuffer;
		Djinn::Buffer _indexBuffer;
		std::vector<Djinn::Buffer> _uniformBuffers;

		VkImage textureImage;
		uint32_t m_mipLevels;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;

		// MSAA images
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // default to 1 sample

		Djinn::Image colorImage;
		Djinn::Image depthImage;


		// model info
		std::vector<Vertex> vertices;
		std::vector<uint32_t> vertexIndices;

	};
}

#endif // DJINN_VULKAN_ENGINE_INCLUDE_H