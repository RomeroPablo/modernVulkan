#include <algorithm>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Vulkan{
    public:
        void run();
    private:
        GLFWwindow* window;
        uint32_t width = 800;
        uint32_t height = 600;
        vk::raii::Context context;
        vk::raii::Instance instance = nullptr;
        void initWindow();
        void initVulkan();
        void createInstance();
        void renderLoop();
        void close();
};

void Vulkan::run(){
    std::cout << "Running..." << std::endl;
    initWindow();
    initVulkan();
    renderLoop();
    close();
};

void Vulkan::initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Modern Vulkan", nullptr, nullptr);
}

void Vulkan::initVulkan(){
    createInstance();
};

void Vulkan::createInstance(){
    uint32_t glfwExtensionCount{};
    constexpr vk::ApplicationInfo appInfo = {
        .pApplicationName   = "Modern Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "None",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for(size_t i{0uz}; i < glfwExtensionCount; i++){
        if(std::ranges::none_of(extensionProperties, 
            [glfwExtension = glfwExtensions[i]](auto const& extensionProperty){
                return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }
                )){ throw::std::runtime_error("Required GLFW extension not supported: "  
                    + std::string(glfwExtensions[i]));}
    }
    vk::InstanceCreateInfo createInfo = {
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions
    };
};

void Vulkan::renderLoop(){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
    }
}

void Vulkan::close(){ glfwDestroyWindow(window); glfwTerminate(); };

int main(){
    try{
        Vulkan app;
        app.run();
    } catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
