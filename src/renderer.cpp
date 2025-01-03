#include "renderer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <algorithm>

#ifdef _DEBUG
const bool enableValidationLayers = true;
#else 
const bool enableValidationLayers = false;
#endif // _DEBUG

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void renderer_t::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
    return;
}

void renderer_t::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}


void renderer_t::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void renderer_t::createFontTexture() {
    FT_Library ft;
    FT_Face     face;
    FT_Init_FreeType(&ft);

    const char* font_filepath = "res/fonts/Tinos/Tinos-Regular.ttf";

    FT_New_Face(ft, font_filepath, 0, &face);
    FT_Set_Char_Size(face, 0, fontSize_c << 6, 96, 96);

    int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(glyphsNum_c));
    uint32_t tex_width = 1;
    while (tex_width < max_dim) tex_width <<= 1;
    uint32_t tex_height = tex_width;
    fontTexSize = tex_height;

    uint8_t* pixels = static_cast<uint8_t*>(calloc(tex_width * tex_height, 1));
    uint32_t pen_x = 0, pen_y = 0;

    for (uint16_t i = 0; i < glyphsNum_c; ++i) {
        FT_Load_Char(face, i, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT);
        FT_Bitmap* bmp = &face->glyph->bitmap;

        if (pen_x + bmp->width >= tex_width) {
            pen_x = 0;
            pen_y += ((face->size->metrics.height >> 6) + 1);
        }

        for (uint32_t row = 0; row < bmp->rows; ++row) {
            for (uint32_t col = 0; col < bmp->width; ++col) {
                uint32_t x = pen_x + col;
                uint32_t y = pen_y + row;
                pixels[y * tex_width + x] = bmp->buffer[row * bmp->pitch + col];
            }
        }

        m_info[i].x0 = pen_x;
        m_info[i].y0 = pen_y;
        m_info[i].x1 = pen_x + bmp->width;
        m_info[i].y1 = pen_y + bmp->rows;

        m_info[i].x_off = face->glyph->bitmap_left;
        m_info[i].y_off = face->glyph->bitmap_top;
        m_info[i].advance = face->glyph->advance.x >> 6;

        pen_x += bmp->width + 1;
    }
    pixels[0] = 255;

    VkDeviceSize imageSize = tex_width * tex_height;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(tex_width, tex_height, VK_FORMAT_R8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fontTexture.image, fontTexture.imageMemory);

    transitionImageLayout(fontTexture.image, VK_FORMAT_R8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, fontTexture.image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
    transitionImageLayout(fontTexture.image, VK_FORMAT_R8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    fontTexture.imageView = createImageView(fontTexture.image, VK_FORMAT_R8_SRGB);

    //sampler

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &fontTexture.sampler) != VK_SUCCESS) 
        throw std::runtime_error("failed to create texture sampler!");    
    return;
}


void renderer_t::createTextures(const std::vector<std::string>& _paths) {
    for (const auto& _path : _paths) {
        textureInfos.push_back({});
        int32_t texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels)
            throw std::runtime_error("failed to load texture image!");

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureInfos.back().image, textureInfos.back().imageMemory);

        transitionImageLayout(textureInfos.back().image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureInfos.back().image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureInfos.back().image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        
        textureInfos.back().imageView = createImageView(textureInfos.back().image, VK_FORMAT_R8G8B8A8_SRGB);
        createTextureSampler(textureInfos.back().sampler, VK_FILTER_NEAREST);
    }
    return;
}

void renderer_t::createTextureSampler(VkSampler& _sampler, VkFilter _filter) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = _filter;
    samplerInfo.minFilter = _filter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS)
        throw std::runtime_error("failed to create texture sampler!");

    return;
}



void renderer_t::framebuffResizeCallback(GLFWwindow* _window, int32_t _newX, int32_t _newY) {
    renderer_t* renderer = reinterpret_cast<renderer_t*>(glfwGetWindowUserPointer(_window));
    renderer->winSizeX = _newX;
    renderer->winSizeY = _newY;
    renderer->framebuffResized = true;
    return;
}

void renderer_t::initWindow(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(winSizeX, winSizeY, appName_c, nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffResizeCallback);
	return;
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) 
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo) {
    _createInfo = {};
    _createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    _createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    _createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    _createInfo.pfnUserCallback = debugCallback;
}

bool checkValidationLayerSupport(void) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) 
            return false;
    }

    return true;
}

void renderer_t::createVkInstance(void) {
    if (enableValidationLayers && !checkValidationLayerSupport()) 
        throw std::runtime_error("validation layers requested, but not available!");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName_c;
    appInfo.applicationVersion = version_c;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    return;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void renderer_t::setupDebugMessenger(void) {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
        throw std::runtime_error("Failed to set up debug messenger!");
    
    return;
}

VkImageView renderer_t::createImageView(VkImage _image, VkFormat _format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void renderer_t::createImageViews(void) {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) 
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);

    return;
}

void renderer_t::createWinSurface(void) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) 
        throw std::runtime_error("failed to create window surface!");
    return;
}

queueFamilyIndices_t renderer_t::findQueueFamilies(VkPhysicalDevice _device) {
    queueFamilyIndices_t indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface, &presentSupport);

        if (presentSupport) 
            indices.presentFamily = i;
      
        if (indices.isComplete()) 
            break;

        i++;
    }

    return indices;
}

bool renderer_t::checkDeviceExtensionSupport(VkPhysicalDevice _device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions_c.begin(), deviceExtensions_c.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

swapChainSupportDetails_t renderer_t::querySwapChainSupport(VkPhysicalDevice _device) {
    swapChainSupportDetails_t details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool renderer_t::isDeviceSuitable(VkPhysicalDevice _device) {
    queueFamilyIndices_t indices = findQueueFamilies(_device);

    bool extensionsSupported = checkDeviceExtensionSupport(_device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        swapChainSupportDetails_t swapChainSupport = querySwapChainSupport(_device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(_device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void renderer_t::pickPhysicalDevice(void) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) 
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void renderer_t::createLogicalDevice(void) {
    queueFamilyIndices_t indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_c.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions_c.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else 
        createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) 
        throw std::runtime_error("failed to create logical device!");
    
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    return;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats) {
    for (const auto& availableFormat : _availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
            return availableFormat;
    }
    return _availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes) {
    for (const auto& availablePresentMode : _availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities, GLFWwindow* _window) {
    if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return _capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, _capabilities.minImageExtent.width, _capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void renderer_t::createSwapChain(void) {
    swapChainSupportDetails_t swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queueFamilyIndices_t indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else 
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    return;
}

void renderer_t::createRenderPass(void) {
    {   // render pass
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDescription subpasses[] = { subpass };

        VkSubpassDependency dependencies[] = { dependency };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = subpasses;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = dependencies;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass[0]) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }
    {   // resize -> present pass
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDescription subpasses[] = { subpass };

        VkSubpassDependency dependencies[] = { dependency };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = subpasses;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = dependencies;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass[1]) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }
    return;
}

void renderer_t::createDescriptorSetLayout(void) {
    {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorCount = 1;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.pImmutableSamplers = nullptr;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 1;
        samplerBinding.descriptorCount = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.pImmutableSamplers = nullptr;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding arrSamplerBinding{};
        arrSamplerBinding.binding = 2;
        arrSamplerBinding.descriptorCount = 8;
        arrSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        arrSamplerBinding.pImmutableSamplers = nullptr;
        arrSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboBinding, samplerBinding, arrSamplerBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout[0]) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }
    {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorCount = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.pImmutableSamplers = nullptr;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { samplerBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout[1]) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }
    return;
}

std::vector<char> readFile(const std::string& _filename) {
    std::ifstream file(_filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) 
        throw std::runtime_error("failed to open file!");
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& _code, VkDevice _device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = _code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
        throw std::runtime_error("failed to create shader module!");

    return shaderModule;
}

void renderer_t::createColorTexture(void) {
    //todo: this shit sets up og framebuffer size -> means that it will be upscaled later / should set it up as const
    const auto width = renderFrameBufferSize_x, height = renderFrameBufferSize_y;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapChainImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
        if (vkCreateImage(device, &imageInfo, nullptr, &colorImgs[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
    }

    for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, colorImgs[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &colorImgMemorys[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, colorImgs[i], colorImgMemorys[i], 0);
        colorImgViews[i] = createImageView(colorImgs[i], swapChainImageFormat);
    }

    for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass[0];
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &colorImgViews[i];
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &renderFrameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
        if (vkCreateSampler(device, &samplerInfo, nullptr, &colorSamplers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    return;
}

void renderer_t::createGraphicsPipeline(void) {
    std::vector<char> vertShaderCode = readFile("res/shaders/vertex.spv");
    std::vector<char> fragShaderCode = readFile("res/shaders/fragment.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = vertex_t::getBindingDescription();
    auto attributeDescriptions = vertex_t::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout[0];

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout[0]) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout[0];
    pipelineInfo.renderPass = renderPass[0];
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline[0]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    return;
}

void renderer_t::createPresentGraphicsPipeline(void) {
    std::vector<char> vertShaderCode = readFile("res/shaders/pVertex.spv");
    std::vector<char> fragShaderCode = readFile("res/shaders/pFragment.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = presentVertex_t::getBindingDescription();
    auto attributeDescriptions = presentVertex_t::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout[1];

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout[1]) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout[1];
    pipelineInfo.renderPass = renderPass[1];
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline[1]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    return;
}

void renderer_t::createFramebuffers(void) {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass[0];
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) 
            throw std::runtime_error("failed to create framebuffer!");
    }
    return;
}

void renderer_t::createCommandPool(void) {
    queueFamilyIndices_t queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) 
        throw std::runtime_error("failed to create graphics command pool!");
    return;
}

uint32_t renderer_t::findMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((_typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void renderer_t::createBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = _size;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) 
        throw std::runtime_error("failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, _buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, _properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, _buffer, _bufferMemory, 0);

    return;
}

VkCommandBuffer renderer_t::beginSingleTimeCommands(void) {
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

void renderer_t::endSingleTimeCommands(VkCommandBuffer _commandBuffer) {
    vkEndCommandBuffer(_commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &_commandBuffer);
}

void renderer_t::copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = _size;
    vkCmdCopyBuffer(commandBuffer, _srcBuffer, _dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);

    return;
}

void renderer_t::createVertexBuffers(void) {
    {
        constexpr VkDeviceSize bufferSize = sizeof(vertex_t) * 4 * 65535; //max size of buff
        for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer[i], vertexBufferMemory[i]);
        }
    }
    { // present
        constexpr VkDeviceSize bufferSize = sizeof(presentVertex_t) * 4; //max size of buff
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, presentVertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pVertexBuffer, pVertexBufferMemory);
        copyBuffer(stagingBuffer, pVertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    return;
}

void renderer_t::createIndexBuffer(void) {
    {
        constexpr VkDeviceSize bufferSize = sizeof(uint32_t) * 6 * 65535;
        for (uint32_t i = 0; i < maxFramesInFlight_c; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer[i], indexBufferMemory[i]);
        }
    }
    {
        constexpr VkDeviceSize bufferSize = sizeof(uint32_t) * 6;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, presentIndices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pIndexBuffer, pIndexBufferMemory);

        copyBuffer(stagingBuffer, pIndexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    return;
}

void renderer_t::createUniformBuffers(void) {
    VkDeviceSize bufferSize = sizeof(uniformBuffer_t);

    uniformBuffers.resize(maxFramesInFlight_c);
    uniformBuffersMemory.resize(maxFramesInFlight_c);
    uniformBuffersMapped.resize(maxFramesInFlight_c);

    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }

    return;
}

void renderer_t::createDescriptorPool(void) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight_c);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>((8+1+1)*maxFramesInFlight_c);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 2 * static_cast<uint32_t>(maxFramesInFlight_c);
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) 
        throw std::runtime_error("failed to create descriptor pool!");
    return;
}

void renderer_t::createDescriptorSets(void) {
    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight_c, descriptorSetLayout[0]);
    std::vector<VkDescriptorSetLayout> pLayouts(maxFramesInFlight_c, descriptorSetLayout[1]);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight_c);
    allocInfo.pSetLayouts = layouts.data();

    VkDescriptorSetAllocateInfo pAllocInfo{};
    pAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    pAllocInfo.descriptorPool = descriptorPool;
    pAllocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight_c);
    pAllocInfo.pSetLayouts = pLayouts.data();

    descriptorSets.resize(maxFramesInFlight_c);
    presentDescriptorSets.resize(maxFramesInFlight_c);

    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    if (vkAllocateDescriptorSets(device, &pAllocInfo, presentDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(uniformBuffer_t);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = fontTexture.imageView;
        imageInfo.sampler = fontTexture.sampler;
        
        std::array<VkDescriptorImageInfo, 8> imageInfos = {};
        for(size_t i = 0; i < imageInfos.size(); i++){
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (i >= textureInfos.size()) {
                imageInfos[i].imageView = textureInfos.back().imageView;
                imageInfos[i].sampler = textureInfos.back().sampler;
            }
            else {
                imageInfos[i].imageView = textureInfos[i].imageView;
                imageInfos[i].sampler = textureInfos[i].sampler;
            }
        }

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = imageInfos.size();
        descriptorWrites[2].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = colorImgViews[i];
        imageInfo.sampler = colorSamplers[i];

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = presentDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return;
}

void renderer_t::createCommandBuffers(void) {
    commandBuffers.resize(maxFramesInFlight_c);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) 
        throw std::runtime_error("failed to allocate command buffers!");
    
    return;
}

void renderer_t::createSyncObjects(void) {
    imageAvailableSemaphores.resize(maxFramesInFlight_c);
    renderFinishedSemaphores.resize(maxFramesInFlight_c);
    inFlightFences.resize(maxFramesInFlight_c);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
    return;
}

void renderer_t::initVk(void) {
    createVkInstance();
    setupDebugMessenger();
    createWinSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createColorTexture();
    createPresentGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffers();
    createIndexBuffer();
    createFontTexture();
    createTextures({"res/textures/chars.png", "res/textures/guiWip.png"});
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();

    return;
}

void renderer_t::initRenderer(void) {
    initWindow();
    initVk();

    pushText({ 200, 350 }, "|");
        
    return;
}

void renderer_t::recreateSwapChain(void) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
     }

    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createFramebuffers();
}

void renderer_t::updateUBO(glm::vec2 _cameraPos) {
    uniformBuffer_t ubo{};
    
    ubo.cameraPos = _cameraPos;
    ubo.windowSize = { renderFrameBufferSize_x, renderFrameBufferSize_y };       

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    return;
}

void renderer_t::recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t _imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    std::array<VkRenderPassBeginInfo, 2> renderPassInfo{};
    renderPassInfo[0].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo[0].renderPass = renderPass[0];
    renderPassInfo[0].framebuffer = renderFrameBuffers[currentFrame];
    renderPassInfo[0].renderArea.offset = { 0, 0 };
    renderPassInfo[0].renderArea.extent = { renderFrameBufferSize_x, renderFrameBufferSize_y };
    renderPassInfo[1].sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo[1].renderPass = renderPass[1];
    renderPassInfo[1].framebuffer = swapChainFramebuffers[_imageIndex];
    renderPassInfo[1].renderArea.offset = { 0, 0 };
    renderPassInfo[1].renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { {{0.0,0.3,0.1, 0.0f}} };
    renderPassInfo[0].clearValueCount = 1;
    renderPassInfo[0].pClearValues = &clearColor;
    renderPassInfo[1].clearValueCount = 1;
    renderPassInfo[1].pClearValues = &clearColor;

    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo[0], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline[0]);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderFrameBufferSize_x;
    viewport.height = (float)renderFrameBufferSize_y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = renderPassInfo[0].renderArea.extent;
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer[currentFrame] };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(_commandBuffer, indexBuffer[currentFrame], 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout[0], 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(_commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(_commandBuffer);
    // present one
    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo[1], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline[1]);

    VkViewport pViewport{};
    pViewport.x = 0.0f;
    pViewport.y = 0.0f;
    pViewport.width = (float)swapChainExtent.width;
    pViewport.height = (float)swapChainExtent.height;
    pViewport.minDepth = 0.0f;
    pViewport.maxDepth = 1.0f;
    vkCmdSetViewport(_commandBuffer, 0, 1, &pViewport);

    VkRect2D pScissor{};
    pScissor.offset = { 0, 0 };
    pScissor.extent = swapChainExtent;
    vkCmdSetScissor(_commandBuffer, 0, 1, &pScissor);

    VkBuffer pVertexBuffers[] = { pVertexBuffer };
    VkDeviceSize pOffsets[] = { 0 };
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, pVertexBuffers, pOffsets);

    vkCmdBindIndexBuffer(_commandBuffer, pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout[1], 0, 1, &presentDescriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(_commandBuffer, static_cast<uint32_t>(presentIndices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(_commandBuffer);
    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void renderer_t::updateRenderBuffers(void) {
    if (vertices.size() <= 0) return;
    void* data = nullptr;
    
    uint64_t buffer_size = sizeof(vertex_t) * vertices.size();
    vkMapMemory(device, vertexBufferMemory[currentFrame], 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, vertexBufferMemory[currentFrame]);

    buffer_size = sizeof(uint32_t) * indices.size();
    vkMapMemory(device, indexBufferMemory[currentFrame], 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), (size_t)buffer_size);
    vkUnmapMemory(device, indexBufferMemory[currentFrame]);

    return;
}

void renderer_t::pushSprite(const sprite_t& _sprite, const position_t& _pos, bool _invertX = false, bool _invertY = false) {
    glm::vec2 pos = { _pos[0], _pos[1] }, size = { _sprite.size[0], _sprite.size[1] };
    glm::vec2 texPos = { _sprite.texPos[0], _sprite.texPos[1] }, texSize = { _sprite.texSize[0], _sprite.texSize[1] };
    if (_invertX) {
        texPos[0] += texSize[0];
        texSize[0] *= -1;
    }

    if (_invertY) {
        texPos[1] += texSize[1];
        texSize[1] *= -1;
    }

    vertex_t v{};
    v.isGui = false;
    //todo: virtual texture id's to make code more readable
    v.texId = _sprite.texId;
    v.pos = pos;
    v.texCoord = texPos;
    vertices.push_back(v);
    v.pos.x += size.x;
    v.texCoord.x += texSize[0];
    vertices.push_back(v);
    v.pos.y += size.y;
    v.texCoord.y += texSize[1];
    vertices.push_back(v);
    v.pos.x -= size.x;
    v.texCoord.x = texPos[0];
    vertices.push_back(v);

    uint16_t FNI = indices.size() / 6 * 4; // first_new_index
    std::vector<uint16_t> indices_to_add = { FNI, (uint16_t)(FNI + 1), (uint16_t)(FNI + 2), (uint16_t)(FNI + 2), (uint16_t)(FNI + 3),FNI };
    indices.insert(indices.end(), indices_to_add.begin(), indices_to_add.end());

    return;
}

void renderer_t::pushQuad(const sprite_t& _sprite, const position_t& _position, bool _isGui) {
    glm::vec2 pos = { _position[0], _position[1] }, size = { _sprite.size[0], _sprite.size[1] };
    
    vertex_t v {};   
    v.isGui = _isGui;
    //todo: virtual texture id's to make code more readable
    v.texId = _sprite.texId;
    v.pos = pos;
    v.texCoord = { _sprite.texPos[0], _sprite.texPos[1] };
    vertices.push_back(v);
    v.pos.x += size.x;
    v.texCoord.x = v.texCoord.x + _sprite.texSize[0];
    vertices.push_back(v);
    v.pos.y += size.y;
    v.texCoord.y = v.texCoord.y + _sprite.texSize[1];
    vertices.push_back(v);
    v.pos.x -= size.x;
    v.texCoord.x = _sprite.texPos[0];
    vertices.push_back(v);

    uint16_t FNI = indices.size() / 6 * 4; // first_new_index
    std::vector<uint16_t> indices_to_add = { FNI, (uint16_t)(FNI + 1), (uint16_t)(FNI + 2), (uint16_t)(FNI + 2), (uint16_t)(FNI + 3),FNI };
    indices.insert(indices.end(), indices_to_add.begin(), indices_to_add.end());

    return;
}

inline void renderer_t::pushQuad(glm::vec2 _pos, glm::vec2 _size, glm::vec2 _texPos, glm::vec2 _texSize, uint32_t _texId, bool _isGui) {

    vertex_t v {};
    v.isGui = _isGui;
    v.texId = _texId;
    v.pos = _pos;
    v.texCoord = _texPos;
    vertices.push_back(v);
    v.pos.x += _size.x;
    v.texCoord.x = v.texCoord.x + _texSize.x;
    vertices.push_back(v);
    v.pos.y += _size.y;
    v.texCoord.y = v.texCoord.y + _texSize.y;
    vertices.push_back(v);
    v.pos.x -= _size.x;
    v.texCoord.x = _texPos.x;
    vertices.push_back(v);

    uint16_t fnwi = indices.size() / 6 * 4; // first_new_index
    std::vector<uint16_t> indices_to_add = { fnwi, (uint16_t)(fnwi + 1), (uint16_t)(fnwi + 2), (uint16_t)(fnwi + 2), (uint16_t)(fnwi + 3),fnwi };
    indices.insert(indices.end(), indices_to_add.begin(), indices_to_add.end());

    return;
}

void renderer_t::pushText(glm::vec2 _pos, const std::string& _str) {
    for (char c : _str) {
        float w = std::abs((int)m_info[c].x1 - m_info[c].x0);
        float h = std::abs((int)m_info[c].y1 - m_info[c].y0);
        float xpos = _pos.x + m_info[c].x_off;
        float ypos = _pos.y + (h - m_info[c].y_off);

        vertex_t v {};
        v.isGui = 1;
        v.texId = fontTextureId;
        v.pos = { xpos,ypos };
        v.texCoord = { m_info[c].x0 / fontTexSize, m_info[c].y1 / fontTexSize };
        vertices.push_back(v);
        v.pos.x += w;
        v.texCoord = { m_info[c].x1 / fontTexSize, m_info[c].y1 / fontTexSize };
        vertices.push_back(v);
        v.pos.y -= h;
        v.texCoord = { m_info[c].x1 / fontTexSize, m_info[c].y0 / fontTexSize };
        vertices.push_back(v);
        v.pos.x -= w;
        v.texCoord = { m_info[c].x0 / fontTexSize, m_info[c].y0 / fontTexSize };
        vertices.push_back(v);

        uint16_t fnwi = indices.size() / 6 * 4; // first_new_index
        std::vector<uint16_t> indices_to_add = { fnwi, (uint16_t)(fnwi + 1), (uint16_t)(fnwi + 2), (uint16_t)(fnwi + 2), (uint16_t)(fnwi + 3),fnwi };
        indices.insert(indices.end(), indices_to_add.begin(), indices_to_add.end());

        _pos.x += m_info[c].advance;
    }

    return;
}

void renderer_t::pushLevel(const levelManager_t& _level){
    for (const auto& pickup : _level.level_.pickups)
        pushSprite(_level.gameData_->pickupData.at(pickup.type).sprite, pickup.position);

    pushSprite(_level.level_.player.sprite, _level.level_.player.spritePosition, _level.level_.player.isTurnedRight);

    for (const auto& enemy : _level.level_.enemies) 
        pushSprite(_level.gameData_->enemyData.at(enemy.type).sprite, enemy.spritePosition, enemy.isTurnedRight);

    for (const auto& projectile : _level.level_.enemyProjectiles) 
        pushSprite(projectile.sprite, projectile.spritePosition);

    for (const auto& projectile : _level.level_.playerProjectiles) 
        pushSprite(projectile.sprite, projectile.spritePosition);
   
    return;
}


#include <format>
void renderer_t::pushGui(const gameManager_t& _mng) {
    for (const auto& guiElement : _mng.layouts_[_mng.currLayout_].guiElements) {
        switch(guiElement.type) { // todo move it to some func (mb inline)
            case GUI_TYPE_SPRITE: {
                const guiSprite_t& guiSprite = *(guiSprite_t*)guiElement.data;

                pushQuad(guiSprite.sprite, guiSprite.position, true);
            }break;
            case GUI_TYPE_BUTTON: {
                const guiButton_t& guiButton = *(guiButton_t*)guiElement.data;

                pushQuad(guiButton.sprite, guiButton.spritePosition, true);
            }break;
            case GUI_TYPE_BAR: {
                const guiBar_t& guiBar = *(guiBar_t*)guiElement.data;
                pushQuad(guiBar.background, guiBar.position, true);
                if (guiBar.horizontal) {
                    pushQuad(glm::vec2{ guiBar.position[0], guiBar.position[1]},
                        glm::vec2{ guiBar.foreground.size[0] * guiBar.filledPercent, guiBar.foreground.size[1]},
                        glm::vec2{ guiBar.foreground.texPos[0], guiBar.foreground.texPos[1] },
                        glm::vec2{ guiBar.foreground.texSize[0] * guiBar.filledPercent, float(guiBar.foreground.texSize[1]) },
                        guiBar.foreground.texId,
                        true);
                } else {
                    pushQuad(glm::vec2{ guiBar.position[0], guiBar.position[1] + guiBar.background.size[1] },
                        glm::vec2{ guiBar.foreground.size[0] , -1.0f * guiBar.foreground.size[1] * guiBar.filledPercent },
                        glm::vec2{ guiBar.foreground.texPos[0], guiBar.foreground.texPos[1] + guiBar.foreground.texSize[1] },
                        glm::vec2{ guiBar.foreground.texSize[0] , -1.0f * guiBar.foreground.texSize[1] * guiBar.filledPercent },
                        guiBar.foreground.texId,
                        true);
                }
                //pushQuad(guiBar.foreground, guiBar.position, true);


            }break;
            case GUI_TYPE_TEXT: {
                const guiText_t& guiText= *(guiText_t*)guiElement.data;
                uint32_t textSize = 0;
                for (char c : guiText.text)
                    textSize += m_info[c].advance;
                switch (guiText.positionType) {
                case TEXT_POSITION_TYPE_LEFT:
                    textSize = 0;
                    break;
                case TEXT_POSITION_TYPE_CENTRE:
                    textSize /= 2;
                    break;
                case TEXT_POSITION_TYPE_RIGHT:
                    textSize = textSize;
                    break;
                }
                pushText({ guiText.position[0] - textSize, guiText.position[1] }, guiText.text);
            }break;
            default: {
                printf("unknown type of guiElement!\n");
            }break;
        }
    
    }

    return;
}

void renderer_t::clearBuffers(void) {
    vertices.clear();
    indices.clear();
    return;
}

void renderer_t::drawFrame(void) {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    updateRenderBuffers();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffResized) {
        framebuffResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight_c;
    clearBuffers();
    return;
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance _instance, VkDebugUtilsMessengerEXT _debugMessenger, const VkAllocationCallbacks* _pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
        func(_instance, _debugMessenger, _pAllocator);
    return;
}

void renderer_t::cleanupSwapChain(void) {
    for (auto framebuffer : swapChainFramebuffers) 
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    for (auto imageView : swapChainImageViews) 
        vkDestroyImageView(device, imageView, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    return;
}

void renderer_t::cleanup(void) {
    vkDeviceWaitIdle(device);
    cleanupSwapChain();

    for (size_t i = 0; i < renderPassesNum_c; i++) {
        vkDestroyImage(device, colorImgs[i], nullptr);
        vkDestroyImageView(device, colorImgViews[i], nullptr);
        vkFreeMemory(device, colorImgMemorys[i], nullptr);
        vkDestroySampler(device, colorSamplers[i], nullptr);
    }
    vkDestroyBuffer(device, pVertexBuffer, nullptr);
    vkDestroyBuffer(device, pIndexBuffer, nullptr);
    vkFreeMemory(device, pVertexBufferMemory, nullptr);
    vkFreeMemory(device, pIndexBufferMemory, nullptr);
    for (size_t i = 0; i < renderPassesNum_c; i++)
        vkDestroyFramebuffer(device, renderFrameBuffers[i], nullptr);
    for (size_t i = 0; i < renderPassesNum_c; i++) {
        vkDestroyPipeline(device, graphicsPipeline[i], nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout[i], nullptr);
        vkDestroyRenderPass(device, renderPass[i], nullptr);
    }
    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        vkDestroyBuffer(device, indexBuffer[i], nullptr);
        vkDestroyBuffer(device, vertexBuffer[i], nullptr);
        vkFreeMemory(device, vertexBufferMemory[i], nullptr);
        vkFreeMemory(device, indexBufferMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    for (size_t i = 0; i < renderPassesNum_c; i++) 
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout[i], nullptr);
    for (size_t i = 0; i < maxFramesInFlight_c; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyImage(device, fontTexture.image, nullptr);
    vkDestroyImageView(device, fontTexture.imageView, nullptr);
    vkFreeMemory(device, fontTexture.imageMemory, nullptr);
    vkDestroySampler(device, fontTexture.sampler, nullptr);
    for (size_t i = 0; i < textureInfos.size();i++) {
        vkDestroyImage(device, textureInfos[i].image, nullptr);
        vkDestroyImageView(device, textureInfos[i].imageView, nullptr);
        vkFreeMemory(device, textureInfos[i].imageMemory, nullptr);
        vkDestroySampler(device, textureInfos[i].sampler, nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers) 
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
    return;
}
