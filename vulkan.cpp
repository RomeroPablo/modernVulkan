module;

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module vulkan;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#define logs(x)                                                                \
  do {                                                                         \
  } while (0)
#else
constexpr bool enableValidationLayers = true;
#define logs(x)                                                                \
  {                                                                            \
    const auto id = std::this_thread::get_id();                                \
    std::cout << "[0x" << std::hex << id << std::dec << "] " << x              \
              << std::endl;                                                    \
  }
#endif

export struct Vulkan {
  GLFWwindow *window = nullptr;
  uint32_t width = 800;
  uint32_t height = 600;
  vk::raii::Context context = {};
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::PhysicalDeviceProperties physicalDeviceProperties{};
  vk::raii::Device device = nullptr;
  vk::raii::Queue graphicsQueue = nullptr;
  float queuePriority = 1.0f;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages = {};
  std::vector<vk::raii::ImageView> swapChainImageViews = {};
  vk::SurfaceFormatKHR swapChainSurfaceFormat = {};
  vk::Extent2D swapChainExtent = {};
  std::vector<const char *> requiredDeviceExtensions = {
      vk::KHRSwapchainExtensionName};

  void run();
  void initWindow();
  void initVulkan();
  void createInstance();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSurface();
  void createSwapChain();
  void createImageViews();
  void createGraphicsPipeline();
  void setupDebugMessenger();
  void renderLoop();
  void close();

  std::vector<const char *> getRequiredInstanceExtensions();
  std::vector<const char *> getRequiredInstanceLayers();
  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
  uint32_t chooseSwapMinImageCount(
      const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);
  bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice);

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL
  debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severityFlags,
                vk::DebugUtilsMessageTypeFlagsEXT typeFlags,
                const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};

bool Vulkan::isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice) {
  bool supportsVulkan1_3 =
      physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
  auto queueFamilies = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics =
      std::ranges::any_of(queueFamilies, [](const auto &qfp) {
        return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
      });
  auto availableDeviceExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions = std::ranges::all_of(
      requiredDeviceExtensions,
      [&availableDeviceExtensions](const auto &requiredDeviceExtension) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension](const auto &availableDeviceExtension) {
              return std::strcmp(availableDeviceExtension.extensionName,
                                 requiredDeviceExtension) == 0;
            });
      });
  auto features = physicalDevice.template getFeatures2<
      vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
  bool supportsRequiredFeatures =
      features.template get<vk::PhysicalDeviceVulkan13Features>()
          .dynamicRendering &&
      features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
          .extendedDynamicState;
  return supportsVulkan1_3 && supportsGraphics &&
         supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Vulkan::run() {
  logs("Running");
  initWindow();
  logs("Initialized Window");
  initVulkan();
  logs("Initialized Vulkan");
  renderLoop();
  logs("Closing");
  close();
}

void Vulkan::pickPhysicalDevice() {
  std::vector<vk::raii::PhysicalDevice> physicalDevices =
      instance.enumeratePhysicalDevices();
  auto const devIter =
      std::ranges::find_if(physicalDevices, [&](const auto &physicalDevice) {
        return isDeviceSuitable(physicalDevice);
      });
  if (devIter == physicalDevices.end()) {
    throw std::runtime_error("failed to find suitable device");
  }
  physicalDevice = *devIter;
}

void Vulkan::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      physicalDevice.getQueueFamilyProperties();
  uint32_t queueIndex = ~0u;
  for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
    if (physicalDevice.getSurfaceSupportKHR(i, *surface) &&
        (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
      queueIndex = i;
      break;
    }
  }
  if (queueIndex == ~0u) {
    throw std::runtime_error(
        "could not find a queue for graphics and presentation");
  }

  vk::StructureChain<vk::PhysicalDeviceFeatures2,
                     vk::PhysicalDeviceVulkan13Features,
                     vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
      featureChain = {
          {},
          {.dynamicRendering = true},
          {.extendedDynamicState = true},
      };
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
      .queueFamilyIndex = queueIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};
  vk::DeviceCreateInfo deviceCreateInfo{
      .pNext = &featureChain
                    .get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount =
          static_cast<uint32_t>(requiredDeviceExtensions.size()),
      .ppEnabledExtensionNames = requiredDeviceExtensions.data()};
  device = vk::raii::Device(physicalDevice, deviceCreateInfo);
  graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
}

void Vulkan::createSurface() {
  VkSurfaceKHR rawSurface;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != 0) {
    throw std::runtime_error("failed to create window surface");
  }
  surface = vk::raii::SurfaceKHR(instance, rawSurface);
}

void Vulkan::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(width, height, "Modern Vulkan", nullptr, nullptr);
}

vk::SurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  const auto formatIt =
      std::ranges::find_if(availableFormats, [](const auto &format) {
        return format.format == vk::Format::eB8G8R8A8Srgb &&
               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
      });
  return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Vulkan::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) {
  assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
    return presentMode == vk::PresentModeKHR::eFifo;
  }));
  return std::ranges::any_of(availablePresentModes,
                             [](const vk::PresentModeKHR value) {
                               return vk::PresentModeKHR::eMailbox == value;
                             })
             ? vk::PresentModeKHR::eMailbox
             : vk::PresentModeKHR::eFifo;
}

vk::Extent2D
Vulkan::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  int framebufferWidth, framebufferHeight;
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

  return {std::clamp<uint32_t>(framebufferWidth,
                               capabilities.minImageExtent.width,
                               capabilities.maxImageExtent.width),
          std::clamp<uint32_t>(framebufferHeight,
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height)};
}

uint32_t Vulkan::chooseSwapMinImageCount(
    const vk::SurfaceCapabilitiesKHR &surfaceCapabilities) {
  auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
  if ((0 < surfaceCapabilities.maxImageCount) &&
      (surfaceCapabilities.maxImageCount < minImageCount)) {
    minImageCount = surfaceCapabilities.maxImageCount;
  }
  return minImageCount;
}

void Vulkan::createSwapChain() {
  vk::SurfaceCapabilitiesKHR surfaceCapabilities =
      physicalDevice.getSurfaceCapabilitiesKHR(*surface);
  swapChainExtent = chooseSwapExtent(surfaceCapabilities);
  uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

  std::vector<vk::SurfaceFormatKHR> availableFormats =
      physicalDevice.getSurfaceFormatsKHR(*surface);
  swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

  std::vector<vk::PresentModeKHR> availablePresentModes =
      physicalDevice.getSurfacePresentModesKHR(*surface);
  vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

  vk::SwapchainCreateInfoKHR swapChainCreateInfo{
      .surface = *surface,
      .minImageCount = minImageCount,
      .imageFormat = swapChainSurfaceFormat.format,
      .imageColorSpace = swapChainSurfaceFormat.colorSpace,
      .imageExtent = swapChainExtent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = true};
  swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
  swapChainImages = swapChain.getImages();
}

void Vulkan::createImageViews() {
  assert(swapChainImageViews.empty());
  vk::ImageViewCreateInfo imageViewCreateInfo{
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainSurfaceFormat.format,
      .components = {vk::ComponentSwizzle::eIdentity,
                     vk::ComponentSwizzle::eIdentity,
                     vk::ComponentSwizzle::eIdentity,
                     vk::ComponentSwizzle::eIdentity},
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .levelCount = 1,
                           .layerCount = 1}};

  for (auto &image : swapChainImages) {
    imageViewCreateInfo.image = image;
    swapChainImageViews.emplace_back(device, imageViewCreateInfo);
  }
}

void Vulkan::createGraphicsPipeline() {
}

void Vulkan::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createGraphicsPipeline();
}

void Vulkan::createInstance() {
  constexpr vk::ApplicationInfo appInfo = {
      .pApplicationName = "Modern Vulkan",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "None",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = vk::ApiVersion14};
  auto requiredExtensions = getRequiredInstanceExtensions();
  auto instanceExtensionProperties =
      context.enumerateInstanceExtensionProperties();
  auto unsupportedExtension = std::ranges::find_if(
      requiredExtensions, [&instanceExtensionProperties](const auto &r) {
        return std::ranges::none_of(
            instanceExtensionProperties, [r](const auto &e) {
              return std::strcmp(e.extensionName, r) == 0;
            });
      });
  if (unsupportedExtension != requiredExtensions.end()) {
    throw std::runtime_error("Required extension not supported: " +
                             std::string(*unsupportedExtension));
  }

  auto requiredLayers = getRequiredInstanceLayers();
  auto instanceLayerProperties = context.enumerateInstanceLayerProperties();
  auto unsupportedLayer = std::ranges::find_if(
      requiredLayers, [&instanceLayerProperties](const auto &r) {
        return std::ranges::none_of(
            instanceLayerProperties,
            [r](const auto &l) { return std::strcmp(l.layerName, r) == 0; });
      });
  if (unsupportedLayer != requiredLayers.end()) {
    throw std::runtime_error("Required layer not supported: " +
                             std::string(*unsupportedLayer));
  }

  vk::InstanceCreateInfo createInfo = {
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
      .ppEnabledLayerNames = requiredLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
      .ppEnabledExtensionNames = requiredExtensions.data(),
  };
  instance = vk::raii::Instance(context, createInfo);
}

std::vector<const char *> Vulkan::getRequiredInstanceExtensions() {
  uint32_t glfwExtensionCount{};
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  if (enableValidationLayers) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  }
  return extensions;
}

std::vector<const char *> Vulkan::getRequiredInstanceLayers() {
  std::vector<const char *> layers{};
  if (enableValidationLayers) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
  }
  return layers;
}

void Vulkan::renderLoop() {
  logs("Entering Render Loop");
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

void Vulkan::close() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severityFlags,
    vk::DebugUtilsMessageTypeFlagsEXT typeFlags,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  std::cerr << "validation layer: type " << to_string(typeFlags)
            << " msg: " << pCallbackData->pMessage << std::endl;
  return vk::False;
}

void Vulkan::setupDebugMessenger() {
  if (!enableValidationLayers) {
    return;
  }
  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
      .messageSeverity = severityFlags,
      .messageType = messageTypeFlags,
      .pfnUserCallback = &debugCallback};
  debugMessenger =
      instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}
