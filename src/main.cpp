#include "Application.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>

std::unique_ptr<ApplicationBase> CreateDemo(const std::string& name)
{
	if (name == "model")
	{
		return std::make_unique<ModelApplication>();
	}
	else if (name == "particle")
	{
		return std::make_unique<ParticleApplication>();
	}
	return nullptr;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage:\n"
            << "  .\\Vulkan_Base_Demos.exe model\n"
            << "  .\\Vulkan_Base_Demos.exe particle\n";
        return EXIT_FAILURE;
    }

    auto app = CreateDemo(argv[1]);
    if (!app)
    {
        std::cerr << "Invalid demo name" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        app->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}



