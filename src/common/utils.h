#pragma once

#define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <GLFW/glfw3.h>
#include<glm/glm.hpp>//引入线性代数相关类型
#include<glm/gtc/matrix_transform.hpp>//包含用于生成模型转换、视图转换和投影转换的函数
#include <vector>
#include <array>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../thirdparty/tiny/tiny_obj_loader.h"

#include <filesystem>
#define NOMINMAX
#include <windows.h>
#include <string>
namespace fs = std::filesystem;


const int MAX_FRAMES_IN_FLIGHT = 2;

const int MAX_PARTICLES = 256 * 256;

enum class DrawFrameResult
{
	Success,        // 渲染呈现全部成功
	SwapchainOutOfDate, // 交换链完全过时，需要重建
	SwapchainSuboptimal, // 交换链参数不匹配，建议重建
	FatalError      // 其他致命错误
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
	}
};

//封装交换链细节结构体
struct SwapChainDetails
{
	VkSurfaceCapabilitiesKHR capabilities;//基本表面功能
	std::vector<VkSurfaceFormatKHR> formats;//表面格式
	std::vector<VkPresentModeKHR> presentModes;//可用演示模式
};

//context依赖结构体
struct RenderContextDeps
{
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkSurfaceKHR& surface;
	GLFWwindow* const window; 
	const VkSampleCountFlagBits& msaaSamples;
	const VkQueue& graphicsQueue;
	const VkQueue& presentQueue;
	const VkQueue& computeQueue;

	RenderContextDeps(
		const VkDevice& device, 
		const VkPhysicalDevice& physicalDevice, 
		const VkSurfaceKHR& surface, 
		GLFWwindow* window,
		const VkSampleCountFlagBits& msaaSamples,
		const VkQueue& graphicsQueue,
		const VkQueue& presentQueue,
		const VkQueue& computeQueue
	): device(device), physicalDevice(physicalDevice), surface(surface), window(window), msaaSamples(msaaSamples), graphicsQueue(graphicsQueue), presentQueue(presentQueue), computeQueue(computeQueue)
	{
		
	}
};

struct SwapchainDeps
{
	const VkSwapchainKHR& swapChain;
	const VkFormat& swapChainImageFormat;
	const VkExtent2D& swapChainExtent;
	const std::vector<VkImageView>& swapChainImageViews;
	const std::vector<VkImage>& swapChainImages;

	SwapchainDeps(
		const VkSwapchainKHR& swapchain,
		const VkFormat& swapChainImageFormat,
		const VkExtent2D& swapChainExtent,
		const std::vector<VkImageView>& swapChainImageViews,
		const std::vector<VkImage>& swapChainImages
	): swapChain(swapchain), swapChainImageFormat(swapChainImageFormat), swapChainExtent(swapChainExtent), swapChainImageViews(swapChainImageViews), swapChainImages(swapChainImages)
	{

	}
};

struct CommandDeps
{
	const VkCommandPool& commandPool;
	const std::vector<VkCommandBuffer>& commandBuffers;
	
	CommandDeps(const VkCommandPool& commandPool, const std::vector<VkCommandBuffer>& commandBuffers):commandPool(commandPool), commandBuffers(commandBuffers){}
};

struct RenderPassDeps
{
	const VkRenderPass& renderPass;
	const std::vector<VkFramebuffer>& swapChainFramebuffers;

	RenderPassDeps(const VkRenderPass& renderPass, const std::vector<VkFramebuffer>& swapChainFramebuffers)
		:renderPass(renderPass), swapChainFramebuffers(swapChainFramebuffers)
	{

	}
};

struct SyncDeps
{
	const std::vector<VkSemaphore>& imageAvailableSemaphores;
	const std::vector<VkSemaphore>& renderFinishedSemaphores;
	const std::vector<VkFence>& inFlightFences;

	SyncDeps(const std::vector<VkSemaphore>& imageAvailableSemaphores,
		const std::vector<VkSemaphore>& renderFinishedSemaphores,
		const std::vector<VkFence>& inFlightFences)
		:imageAvailableSemaphores(imageAvailableSemaphores), 
		renderFinishedSemaphores(renderFinishedSemaphores), 
		inFlightFences(inFlightFences)
	{

	}
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	//顶点描述
	static VkVertexInputBindingDescription getBingdingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);//指定条目之间的字节数
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//在每个顶点之后移动到下一数据条目

		return bindingDescription;
	}

	//属性描述
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		//位置描述
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		//颜色描述
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		//纹理描述
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	//哈希相等性测试
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};
//哈希函数
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Particle, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Particle, color);

		return attributeDescriptions;
	}

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Particle);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
};



VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

SwapChainDetails querySwapChainSupported(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice device, VkPhysicalDevice physicalDevice);

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

fs::path GetExeDirectory();

std::vector<char> readFile(const fs::path& filePath);

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
