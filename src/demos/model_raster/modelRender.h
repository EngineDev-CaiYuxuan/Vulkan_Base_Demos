#pragma once

#include "common/utils.h"


class ModelRender
{
public:
	ModelRender(const RenderContextDeps& rcd, const SwapchainDeps& scd, const CommandDeps& cd, const RenderPassDeps& rpd, const SyncDeps& sd);
	void initialize();
	void cleanup();
	DrawFrameResult drawFrame();

private:
	//辅助函数
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice device);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	

	//初始化函数
	void loadModel();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createIndexBuffer();
	void createVertexBuffer();
	void createUniformBuffers();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createGraphicsPipeline();
	//运行函数
	void updateUniformBuffer(uint32_t currentImage);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	
private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkQueue& graphicsQueue;
	const VkQueue& presentQueue;
	const VkSampleCountFlagBits& msaaSamples;

	const VkSwapchainKHR& swapChain;
	const VkExtent2D& swapChainExtent;
	
	const VkCommandPool& commandPool;
	const std::vector<VkCommandBuffer>& commandBuffers;

	const VkRenderPass& renderPass;
	const std::vector<VkFramebuffer>& swapChainFramebuffers;

	const std::vector<VkSemaphore>& imageAvailableSemaphores;
	const std::vector<VkSemaphore>& renderFinishedSemaphores;
	const std::vector<VkFence>& inFlightFences;


private:
	const std::string MODEL_PATH = "assets/models/viking_room.obj";
	const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

	uint32_t currentFrame = 0;

	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<Vertex> vertices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	std::vector<uint32_t> indices;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
};