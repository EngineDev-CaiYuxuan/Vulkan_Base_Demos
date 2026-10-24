#pragma once

#include "common/utils.h"


class ParticleSystem
{
public:
	ParticleSystem(const RenderContextDeps& rcd, const SwapchainDeps& scd, const CommandDeps& cd, const RenderPassDeps& rpd, const SyncDeps& sd);
	void initialize();
	void cleanup();
	DrawFrameResult drawFrame();

	//辅助函数
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice device);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	//初始化函数
	void createUniformBuffers();
	void createParticleBuffer();
	void createComputeDescriptorSetLayout();
	void createComputeDescriptorSetPool();
	void createComputeDescriptorSets();
	void createComputeCommandBuffer();
	void createComputePipeline();
	void createComputeSyncObjects();
	void createParticlePipeline();
	//运行函数
	void updataUniformBuffer(uint32_t currentImage);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recordComputeCommandBuffer(VkCommandBuffer computeCommandBuffer, uint32_t currentFrame);


	
private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkQueue& graphicsQueue;
	const VkQueue& computeQueue;
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
	const uint32_t PARTICLE_WIDTH = 800; 
	const uint32_t PARTICLE_HEIGHT = 600;

	uint32_t currentFrame = 0;

	

	//统一缓冲
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	//乒乓双缓冲
	std::vector<VkBuffer> shaderStorageBuffers;
	std::vector<VkDeviceMemory> shaderStorageBufferMemory;

	//描述符相关
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorPool computeDescriptorPool;
	std::vector<VkDescriptorSet> computeDescriptorSets;

	//计算管线相关
	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;
	std::vector<VkCommandBuffer> computeCommandBuffers;

	//图形管线
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	//计算管线信号量
	std::vector<VkFence> computeInFlightFences;
	std::vector<VkSemaphore> computeFinishedSemaphores;
};