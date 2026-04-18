#include <algorithm>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#define logs(x) do {} while(0)
#else
constexpr bool enableValidationLayers = true;
#define logs(x){ const auto id = std::this_thread::get_id(); std::cout << "[0x" << std::hex << id << std::dec << "] " << x << std::endl; }
#endif

struct Vulkan{
        GLFWwindow* window;
        uint32_t width = 800;
        uint32_t height = 600;
        vk::raii::Context context{};
        vk::raii::Instance instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::PhysicalDeviceProperties physicalDeviceProperties{};
        void run();
        void initWindow();
        void initVulkan();
        void createInstance();
        void pickPhysicalDevice();
        void setupDebugMessenger();
        void renderLoop();
        void close();

        std::vector<const char*> getRequiredInstanceExtensions();
        std::vector<const char*> getRequiredInstanceLayers();
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severityFlags,
                                                            vk::DebugUtilsMessageTypeFlagsEXT typeFlags,
                                                            const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                            void* pUserData );
};
void Vulkan::run(){
    logs("Running");
    initWindow(); logs("Initialized Window");
    initVulkan(); logs("Initialized Vulkan");
    renderLoop(); logs("Closing");
    close();
};

void Vulkan::pickPhysicalDevice(){
    auto physicalDeviceList = instance.enumeratePhysicalDevices();
    physicalDevice = physicalDeviceList[0];
    physicalDeviceProperties = physicalDevice.getProperties();
};

void Vulkan::initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Modern Vulkan", nullptr, nullptr);
}

void Vulkan::initVulkan(){
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
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
    auto requiredExtensions = getRequiredInstanceExtensions();
    auto instanceExtensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedExtension = std::ranges::find_if(requiredExtensions, [&instanceExtensionProperties](const auto& r){
                         return std::ranges::none_of(instanceExtensionProperties, [r](const auto& e){
                         return strcmp(e.extensionName, r) == 0;}); });

    auto requiredLayers = getRequiredInstanceLayers();
    auto instanceLayerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayer = std::ranges::find_if(requiredLayers, [&instanceLayerProperties](const auto& r){
                     return std::ranges::none_of(instanceLayerProperties, [r](const auto& l){
                     return strcmp(l.layerName, r) == 0; }); });
    if(unsupportedLayer != requiredLayers.end())
        throw::std::runtime_error("Required Layer not supported: " + std::string(*unsupportedLayer));

    vk::InstanceCreateInfo createInfo = {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };
    instance = vk::raii::Instance(context, createInfo);
};

std::vector<const char*> Vulkan::getRequiredInstanceExtensions(){
    uint32_t glfwExtensionCount{};
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if(enableValidationLayers) extensions.push_back(vk::EXTDebugUtilsExtensionName);
    return extensions;
};

std::vector<const char*> Vulkan::getRequiredInstanceLayers(){
    std::vector<const char*> layers{};
    if(enableValidationLayers) layers.push_back("VK_LAYER_KHRONOS_validation");
    return layers;
};

void Vulkan::renderLoop(){
    logs("Entering Render Loop");
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
    } close();
}

void Vulkan::close(){ glfwDestroyWindow(window); glfwTerminate(); };

VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severityFlags,
                                                    vk::DebugUtilsMessageTypeFlagsEXT typeFlags,
                                                    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void* pUserData ){
    std::cerr << "validation layer: type " << to_string(typeFlags) << " msg: " << pCallbackData->pMessage << std::endl;
    return vk::False;
};
void Vulkan::setupDebugMessenger(){
    if(!enableValidationLayers) return;
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                          .messageType     = messageTypeFlags,
                                                                          .pfnUserCallback = &debugCallback};
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
};

int main(){
    try{ Vulkan app; app.run(); } 
    catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
