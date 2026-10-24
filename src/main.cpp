#include "common/utils.h"
#include "common/VulkanContext.h"
#include "common/swapchainManager.h"
#include "common/RenderPassManager.h"
#include "common/commandManager.h"
#include "common/syncManager.h"
#include "demos/model_raster/modelRender.h"
#include "demos/particle_system/particleSystem.h"


#include <fstream>
#include <filesystem> // debug

#define MODEL 0;
#define PARTICLE 1;


class ModelApplication
{
public:
	void run()
	{
		initialize();
		mainLoop();
		cleanup();
	}

private:
	void initialize()
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

	void mainLoop()
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

	void cleanup()
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

public:
	
	void cleanupSwapChain()
	{
		rpm->cleanForRecreate();
		scm->cleanForRecreate();
	}
	void recreateSwapChain()
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

private:
	VulkanContext* vc;
	SwapchainManager* scm;
	RenderPassManager* rpm;
	CommandManager* cm;
	SyncManager* sym;
	ModelRender* render;
};

class ParticleApplication
{
public:
	void run()
	{
		initialize();
		mainLoop();
		cleanup();
	}

private:
	void initialize()
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

	void mainLoop()
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

	void cleanup()
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

public:

	void cleanupSwapChain()
	{
		rpm->cleanForRecreate();
		scm->cleanForRecreate();
	}
	void recreateSwapChain()
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

private:
	VulkanContext* vc;
	SwapchainManager* scm;
	RenderPassManager* rpm;
	CommandManager* cm;
	SyncManager* sym;
	ParticleSystem* render;
};

int main()
{
	//ModelApplication app;
	ParticleApplication app;
	
	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



