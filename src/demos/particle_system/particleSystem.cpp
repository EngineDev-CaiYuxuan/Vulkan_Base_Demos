#include "particleSystem.h"

#include <random>
#include <array>
#include <chrono>//包含精确计时的函数


ParticleSystem::ParticleSystem(const RenderContextDeps& rcd, const SwapchainDeps& scd, const CommandDeps& cd, const RenderPassDeps& rpd, const SyncDeps& sd)
	:physicalDevice(rcd.physicalDevice), device(rcd.device), graphicsQueue(rcd.graphicsQueue), computeQueue(rcd.computeQueue), presentQueue(rcd.presentQueue), msaaSamples(rcd.msaaSamples),
	swapChain(scd.swapChain), swapChainExtent(scd.swapChainExtent),
	commandPool(cd.commandPool), commandBuffers(cd.commandBuffers),
	renderPass(rpd.renderPass), swapChainFramebuffers(rpd.swapChainFramebuffers),
	imageAvailableSemaphores(sd.imageAvailableSemaphores), renderFinishedSemaphores(sd.renderFinishedSemaphores), inFlightFences(sd.inFlightFences)

{

}

void ParticleSystem::initialize()
{
	createUniformBuffers();
	createParticleBuffer();
	createComputeDescriptorSetLayout();
	createComputeDescriptorSetPool();
	createComputeDescriptorSets();
	createComputeCommandBuffer();
	createComputePipeline();
	createComputeSyncObjects();
	createParticlePipeline();
}

void ParticleSystem::cleanup()
{
	for (auto fence : computeInFlightFences)
	{
		vkDestroyFence(device, fence, nullptr);
	}

	for (auto semaphore : computeFinishedSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	vkFreeCommandBuffers(device, commandPool, computeCommandBuffers.size(), computeCommandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyPipeline(device, computePipeline, nullptr);
	vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);

	vkDestroyDescriptorPool(device, computeDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);

	for (auto bufferMemory : shaderStorageBufferMemory)
	{
		vkFreeMemory(device, bufferMemory, nullptr);
	}

	for (auto buffer : shaderStorageBuffers)
	{
		vkDestroyBuffer(device, buffer, nullptr);
	}

	for (auto bufferMemory : uniformBuffersMemory)
	{
		vkFreeMemory(device, bufferMemory, nullptr);
	}
	for (auto buffer : uniformBuffers)
	{
		vkDestroyBuffer(device, buffer, nullptr);
	}
}


//绘制函数
DrawFrameResult ParticleSystem::drawFrame()
{
	//等待前一帧完成
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	//VK_TRUE:等待所有栅栏 UINT64_MAX:不考虑等待超时

	//从交换链获取图像
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); //向交换链申请一张空白图像

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return DrawFrameResult::SwapchainOutOfDate;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return DrawFrameResult::FatalError;
	}

	//延迟重置栅栏
	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	vkResetFences(device, 1, &computeInFlightFences[currentFrame]);

	updataUniformBuffer(currentFrame);//更新统一缓冲区


	//计算管线部分

	//重置命令
	vkResetCommandBuffer(computeCommandBuffers[currentFrame], 0);
	recordComputeCommandBuffer(computeCommandBuffers[currentFrame], currentFrame);

	//提交计算任务
	VkSubmitInfo computeSubmitInfo{};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];
	//计算任务完成后触发computeFinishedSemaphore
	computeSubmitInfo.signalSemaphoreCount = 1;
	computeSubmitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

	if (vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit compute command buffer!");
	}


	//图形管线部分

	//重录命令缓冲区
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	//提交命令缓冲区
	VkSubmitInfo submitInfo{};//队列通过这个结构体来提交和同步
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//指定执行开始前要等待的信号量和在哪个阶段等待
	VkSemaphore waitSemaphore[] = { imageAvailableSemaphores[currentFrame], computeFinishedSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	//指定实际提交命令的缓冲区
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	//指定完成后要发出的信号量
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	//提交结构体到图形队列
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("fialed to submit draw command buffer!");
	}

	//将结果递交回交换链

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	//指定演示发生之前需要等待的信号量
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	//指定交换链及其图像索引（可以同时指定多个交换链 但几乎是单个）
	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	//?
	presentInfo.pResults = nullptr; // Optional

	//提交将图像呈现给交换链的请求
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return DrawFrameResult::SwapchainOutOfDate;
	}
	if (result == VK_SUBOPTIMAL_KHR) {
		return DrawFrameResult::SwapchainSuboptimal;
	}
	if (result != VK_SUCCESS) {
		return DrawFrameResult::FatalError;
	}

	//完成后前进到下一帧
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return DrawFrameResult::Success;
}

//记录命令缓冲区
void ParticleSystem::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//填充开始结构体
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;//指定将如何使用该命令缓冲区
	beginInfo.pInheritanceInfo = nullptr;//该参数只与辅助缓冲区有关
	//提交结构体
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	//开始渲染通道
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;//需要绑定的附件
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];//绑定帧缓冲
	//？定义渲染区域的大小
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;//?
	//定义清除值
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues.data();


	//开始渲染通道
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	//第三参数表示渲染通道命令将从辅助命令缓冲区执行

	//绑定图形管线
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkDeviceSize offsets[] = { 0 };
	uint32_t readIndex = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[readIndex], offsets);

	//设置动态视口和剪刀状态
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	//
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//发出绘制命令
	vkCmdDraw(commandBuffer, MAX_PARTICLES, 1, 0, 0);

	//结束渲染通道
	vkCmdEndRenderPass(commandBuffer);

	//完成命令缓冲区的记录
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}
//粒子缓冲兼顶点缓冲创建函数
void ParticleSystem::createParticleBuffer()
{
	// Initialize particles
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
	// Initial particle positions on a circle
	std::vector<Particle> particles(MAX_PARTICLES);
	for (auto& particle : particles) {
		float r = 0.25f * sqrt(rndDist(rndEngine));
		float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
		float x = r * cos(theta) * PARTICLE_HEIGHT / PARTICLE_WIDTH;
		float y = r * sin(theta);
		particle.position = glm::vec2(x, y);
		particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
		particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 0.15f);
	}


	VkDeviceSize bufferSize = sizeof(Particle) * MAX_PARTICLES;
	shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	shaderStorageBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);

	//创建暂存缓冲区
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, //数据传输源标志 允许DMA引擎从这里读取数据 SRC与DST是一对 DST是允许DMA写入
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | //cpu地址映射标志 允许内存被映射到cpu内存 该标志并不指定内存的物理位置归属cpu还是gpu 仅声明可以被cpu映射
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //自动同步开关 内存一致性保证 永远和HOST_VISIBLE搭配使用
		stagingBuffer,
		stagingBufferMemory,
		device
	);

	//划出映射内存并将数据拷贝进入
	void* data; //void*类型表示这是原始字节指针 暂未绑定任何类型
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data); //将暂存缓冲映射到主机内存
	memcpy(data, //此处需要一个内存指针 不能直接写stagingBufferMemory 这只是一种对象句柄 并不能直接用于操作内存
		particles.data(), bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		//创建本地缓冲
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //存储缓冲标志
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | //顶点缓冲标志
			VK_BUFFER_USAGE_TRANSFER_DST_BIT, //传输写入标志
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //显存本地存储标志 表示该内存的物理地址位于显存
			shaderStorageBuffers[i],
			shaderStorageBufferMemory[i],
			device
		);

		//将暂存缓冲拷贝至本地缓冲
		copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
	}

	//销毁暂存缓冲
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

//创建描述符布局
void ParticleSystem::createComputeDescriptorSetLayout()
{
	//填写描述符布局绑定
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
	//uniformBuffer布局绑定
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].pImmutableSamplers = nullptr; //不可变采样器 暂时无关
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	//ssboBuffer布局绑定
	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].pImmutableSamplers = nullptr; //不可变采样器
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	//ssboBuffer布局绑定
	layoutBindings[2].binding = 2;
	layoutBindings[2].descriptorCount = 1;
	layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[2].pImmutableSamplers = nullptr; //不可变采样器
	layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	//创建描述符集布局
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 3;
	createInfo.pBindings = layoutBindings.data();
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create copute descriptor set layout!");
	}
}
//创建描述符池
void ParticleSystem::createComputeDescriptorSetPool()
{
	//获取PoolSize 池子的总容量
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

	//创建描述符池
	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = 2;
	if (vkCreateDescriptorPool(device, &createInfo, nullptr, &computeDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}
//创建描述符集
void ParticleSystem::createComputeDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = computeDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate compute descriptor sets!");
	}

	//配置描述符集
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo uboBufferInfo{};
		uboBufferInfo.buffer = uniformBuffers[i];
		uboBufferInfo.offset = 0;
		uboBufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorBufferInfo ssboInInfo{};
		ssboInInfo.buffer = shaderStorageBuffers[i];
		ssboInInfo.offset = 0;
		ssboInInfo.range = sizeof(Particle) * MAX_PARTICLES;

		VkDescriptorBufferInfo ssboOutInfo{};
		ssboOutInfo.buffer = shaderStorageBuffers[(i + 1) % 2];
		ssboOutInfo.offset = 0;
		ssboOutInfo.range = sizeof(Particle) * MAX_PARTICLES;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		//写入绑定
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = computeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &uboBufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = computeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &ssboInInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = computeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &ssboOutInfo;

		//更新描述符集
		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr
		);
	}
}

//创建计算管线 计算管线 = 计算着色器 + 管线布局
void ParticleSystem::createComputePipeline()
{
	//创建管线布局
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create compute pipeline!");
	}

	//获取着色器字节码
	auto computeShaderCode = readFile("shaders/comp.spv");
	//包装着色器字节码
	VkShaderModule compShaderModule = createShaderModule(computeShaderCode, device);
	//创建着色器阶段
	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;//指定着色器的可使用阶段
	computeShaderStageInfo.module = compShaderModule;
	computeShaderStageInfo.pName = "main";//入口点 即要调用的函数

	//创建管线
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = computePipelineLayout;
	pipelineInfo.stage = computeShaderStageInfo;
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create compte pipeline!");
	}

	//VkShaderModule对象使用后要及时销毁
	vkDestroyShaderModule(device, compShaderModule, nullptr);
}

void ParticleSystem::createComputeCommandBuffer()
{
	computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	//
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;//绑定命令池子
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//指定分配的命令缓冲区是主缓冲区还是辅助缓冲区
	allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();//分配命令缓冲区的数量}
	if (vkAllocateCommandBuffers(device, &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate compute command buffers!");
	}
}

//录制计算命令缓冲
void ParticleSystem::recordComputeCommandBuffer(VkCommandBuffer computeCommandBuffer, uint32_t currentFrame)
{
	//开始录制命令
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//所有命令必须包含在vkBeginCommandBuffer和vkEndCommandBuffer之间
	if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin compute command buffer!");
	}

	//绑定计算管线
	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	//绑定描述符集
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame], 0, 0);

	//计算命令
	vkCmdDispatch(computeCommandBuffer, MAX_PARTICLES / 256, 1, 1);

	//结束录制
	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to end compute command buffer!");
	}
}

//计算信号量创建函数
void ParticleSystem::createComputeSyncObjects()
{
	computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //指定初始状态为已触发 防止CPU被永久堵塞

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if ((vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS) ||
			(vkCreateFence(device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS))
		{
			throw std::runtime_error("failed to create compute synchronization objects for a frame!");
		}
	}
}

VkCommandBuffer ParticleSystem::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void ParticleSystem::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void ParticleSystem::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDevice device)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	//关联内存与缓冲区
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
//复制缓冲区
void ParticleSystem::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

//粒子系统专用图形管线
void ParticleSystem::createParticlePipeline()
{
	//管线布局
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = VK_NULL_HANDLE; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create particle pipeline layout!");
	}


	//获取着色器字节码
	auto vertShaderCode = readFile("shaders/pVert.spv");
	auto fragShaderCode = readFile("shaders/pFrag.spv");

	//包装着色器字节码
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

	//创建分配着色阶段
	//顶点着色器
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;//指定着色器的可使用阶段
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";//入口点 即要调用的函数
	//片段着色器
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;//指定着色器的可使用阶段
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";//入口点 即要调用的函数
	//创建数组包含上述两个结构体
	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderStageInfo,
		fragShaderStageInfo,
	};


	//动态状态
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//顶点输入
	auto bindingDescription = Particle::getBindingDescription();
	auto attributeDescriptions = Particle::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	//输入汇编
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; //拓扑
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//视口和裁剪矩阵
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	//指定裁剪矩阵
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	//指定计数
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	//光栅化器
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;//是否钳制超出近平面和远平面的画面
	rasterizer.rasterizerDiscardEnable = VK_FALSE;//？
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;//如何为几何图形生成片段
	rasterizer.lineWidth = 1.0f;//描述线段的粗细
	rasterizer.cullMode = VK_CULL_MODE_NONE;//剔除模式
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//顶点顺序
	//偏移深度值 用于阴影映射 此处不启用
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//多重采样
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//深度和模板测试
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	//颜色混合
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	//
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;//引用其他渲染管道
	pipelineInfo.basePipelineIndex = -1;//子渲染管道索引 本次不使用

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//在该函数末端销毁对象
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void ParticleSystem::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i], device);

		vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}
//更新Uniform缓冲区数据
void ParticleSystem::updataUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	//构造变换矩阵
	UniformBufferObject ubo{};
	ubo.model = glm::mat4(1.0f);
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(0.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;//反转Y坐标

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));//将对象复制到uniform缓冲区
}