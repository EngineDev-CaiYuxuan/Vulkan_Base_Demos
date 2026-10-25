#include "application.h"

void ModelApplication::run()
{
	initialize();
	mainLoop();
	cleanup();
}

void ModelApplication::initialize()
{
	vc = new VulkanContext;
	vc->initialize();
	RenderContextDeps rcd = vc->getContextDeps();

	scm = new SwapchainManager(rcd);
	scm->initialize();
	SwapchainDeps scd = scm->getSwapchainDeps();

	rpm = new RenderPassManager(rcd, scd);
	rpm->initialize();
	RenderPassDeps rpd = rpm->getRenderPassDeps();

	cm = new CommandManager(rcd);
	cm->initialize();
	CommandDeps cd = cm->getCommandDeps();

	sym = new SyncManager(rcd);
	sym->initialize();
	SyncDeps syd = sym->getSyncDeps();

	render = new ModelRender(rcd, scd, cd, rpd, syd);
	render->initialize();
}

void ModelApplication::mainLoop()
{
	auto window = vc->getWindow();
	auto& framebufferResized = vc->getFrameBool();
	auto& device = vc->getDevice();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}

		auto result = render->drawFrame();

		if (result == DrawFrameResult::SwapchainOutOfDate
			|| result == DrawFrameResult::SwapchainSuboptimal)
		{
			recreateSwapChain();
			continue;
		}
		if (result == DrawFrameResult::FatalError) {
			throw std::runtime_error("render failed");
		}
	}

	vkDeviceWaitIdle(device);
}

void ModelApplication::cleanup()
{
	render->cleanup();
	delete render;
	sym->cleanup();
	delete sym;
	cm->cleanup();
	delete cm;
	rpm->cleanup();
	delete rpm;
	scm->cleanup();
	delete scm;
	vc->cleanup();
	delete vc;
}

void ModelApplication::cleanupSwapChain()
{
	rpm->cleanForRecreate();
	scm->cleanForRecreate();
}
void ModelApplication::recreateSwapChain()
{
	//处理窗口最小化（暂停处理）
	vc->recreateWindow();

	//清理旧交换链
	cleanupSwapChain();

	//重新创建调用函数
	scm->createSwapChain();
	scm->createImageViews();
	rpm->createColorResources();
	rpm->createDepthResources();
	rpm->createFramebuffers();
}


//ParticleSystem
void ParticleApplication::run()
{
	initialize();
	mainLoop();
	cleanup();
}

void ParticleApplication::initialize()
{
	vc = new VulkanContext;
	vc->initialize();
	RenderContextDeps rcd = vc->getContextDeps();

	scm = new SwapchainManager(rcd);
	scm->initialize();
	SwapchainDeps scd = scm->getSwapchainDeps();

	rpm = new RenderPassManager(rcd, scd);
	rpm->initialize();
	RenderPassDeps rpd = rpm->getRenderPassDeps();

	cm = new CommandManager(rcd);
	cm->initialize();
	CommandDeps cd = cm->getCommandDeps();

	sym = new SyncManager(rcd);
	sym->initialize();
	SyncDeps syd = sym->getSyncDeps();

	render = new ParticleSystem(rcd, scd, cd, rpd, syd);
	render->initialize();
}

void ParticleApplication::mainLoop()
{
	auto window = vc->getWindow();
	auto& framebufferResized = vc->getFrameBool();
	auto& device = vc->getDevice();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}

		auto result = render->drawFrame();

		if (result == DrawFrameResult::SwapchainOutOfDate
			|| result == DrawFrameResult::SwapchainSuboptimal)
		{
			recreateSwapChain();
			continue;
		}
		if (result == DrawFrameResult::FatalError) {
			throw std::runtime_error("render failed");
		}
	}

	vkDeviceWaitIdle(device);
}

void ParticleApplication::cleanup()
{
	render->cleanup();
	delete render;
	sym->cleanup();
	delete sym;
	cm->cleanup();
	delete cm;
	rpm->cleanup();
	delete rpm;
	scm->cleanup();
	delete scm;
	vc->cleanup();
	delete vc;
}

void ParticleApplication::cleanupSwapChain()
{
	rpm->cleanForRecreate();
	scm->cleanForRecreate();
}
void ParticleApplication::recreateSwapChain()
{
	//处理窗口最小化（暂停处理）
	vc->recreateWindow();

	//清理旧交换链
	cleanupSwapChain();

	//重新创建调用函数
	scm->createSwapChain();
	scm->createImageViews();
	rpm->createColorResources();
	rpm->createDepthResources();
	rpm->createFramebuffers();
}