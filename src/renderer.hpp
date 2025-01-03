#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "gameManager.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H  
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#include <array>
#include <vector>
#include <optional>
#include <string>

struct queueFamilyIndices_t {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	inline bool isComplete(void) {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct swapChainSupportDetails_t {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct presentVertex_t {
	glm::vec2 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(presentVertex_t);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(presentVertex_t, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(presentVertex_t, texCoord);

		return attributeDescriptions;
	}
};

struct vertex_t {
	glm::vec2 pos;
	glm::vec2 texCoord;
	uint32_t texId;
	uint32_t isGui;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(vertex_t);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(vertex_t, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(vertex_t, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[2].offset = offsetof(vertex_t, texId);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = offsetof(vertex_t, isGui);

		return attributeDescriptions;
	}
};

struct uniformBuffer_t {
	glm::vec2 cameraPos;
	glm::vec2 windowSize;
};

struct imageInfo_t {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler sampler;
};

struct renderer_t {
	const char* appName_c = "deadSurvivors_inator";
	const uint32_t version_c = 01; // 01 = 0.1
	const std::vector<const char*> deviceExtensions_c = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	static constexpr uint8_t maxFramesInFlight_c = 2;
	static constexpr uint8_t renderPassesNum_c = 2;
	static constexpr uint32_t renderFrameBufferSize_x = 400, renderFrameBufferSize_y = 300;

	GLFWwindow* window;
	uint32_t winSizeX = 3*renderFrameBufferSize_x, winSizeY = 3* renderFrameBufferSize_y;
	bool framebuffResized = false;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	// first pass 
	std::array<VkFramebuffer, maxFramesInFlight_c> renderFrameBuffers;
	std::array<VkImage, maxFramesInFlight_c> colorImgs;
	std::array<VkSampler, maxFramesInFlight_c> colorSamplers;
	std::array<VkDeviceMemory, maxFramesInFlight_c> colorImgMemorys;
	std::array<VkImageView, maxFramesInFlight_c> colorImgViews;
	
	std::array<VkRenderPass, renderPassesNum_c> renderPass;
	std::array<VkDescriptorSetLayout, renderPassesNum_c> descriptorSetLayout;
	std::array<VkPipelineLayout, renderPassesNum_c> pipelineLayout;
	std::array<VkPipeline, renderPassesNum_c> graphicsPipeline;

	std::array<VkBuffer, maxFramesInFlight_c> vertexBuffer;
	std::array<VkDeviceMemory, maxFramesInFlight_c> vertexBufferMemory;
	std::array<VkBuffer, maxFramesInFlight_c> indexBuffer;
	std::array<VkDeviceMemory, maxFramesInFlight_c> indexBufferMemory;

	VkBuffer pVertexBuffer;
	VkBuffer pIndexBuffer;
	VkDeviceMemory pVertexBufferMemory;
	VkDeviceMemory pIndexBufferMemory;
	
	VkCommandPool commandPool;

	static constexpr uint32_t fontTextureId = 1234;
	imageInfo_t fontTexture;
	std::vector<imageInfo_t> textureInfos;

	static constexpr uint8_t fontSize_c = 13;
	static constexpr uint32_t glyphsNum_c = 255;
	struct glyphInfo_t {
		int32_t x0, y0, x1, y1;    // coords of glyph in the texture atlas
		int32_t x_off, y_off;    // left & top bearing when rendering
		int32_t advance;            // x advance when rendering
	};
	double fontTexSize;
	glyphInfo_t m_info[glyphsNum_c];

	std::vector<vertex_t> vertices = {};
	std::vector<uint32_t> indices = {};
	//todo move to creating vertex/index buffer
	static constexpr std::array<presentVertex_t,4> presentVertices {
		presentVertex_t{glm::vec2{-1.0f,-1.0f}, glm::vec2{0.0f,0.0f}},
		presentVertex_t{glm::vec2{1.0f,-1.0f}, glm::vec2{1.0f,0.0f}},
		presentVertex_t{glm::vec2{1.0f,1.0f}, glm::vec2{1.0f,1.0f}},
		presentVertex_t{glm::vec2{-1.0f,1.0f}, glm::vec2{0.0f,1.0f}}
	};
	static constexpr std::array<uint32_t,6> presentIndices = {0,1,2, 2,3,0};

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> presentDescriptorSets;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;

	void initRenderer(void);
	
	static void framebuffResizeCallback(GLFWwindow*, int32_t, int32_t);
	void initWindow(void);
	void initVk(void);
	void createWinSurface(void);

	void createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
	void createFontTexture(void);
	void createTextures(const std::vector<std::string>&);
	void createTextureSampler(VkSampler&, VkFilter);

	queueFamilyIndices_t findQueueFamilies(VkPhysicalDevice _device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice _device);
	swapChainSupportDetails_t querySwapChainSupport(VkPhysicalDevice _device);
	uint32_t findMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties);
	bool isDeviceSuitable(VkPhysicalDevice _device);
	void pickPhysicalDevice(void);
	void createLogicalDevice(void);
	void createSwapChain(void);

	void createVkInstance(void);
	void setupDebugMessenger(void);
	void createImageViews(void);
	void createRenderPass(void);
	void createDescriptorSetLayout(void);
	void createColorTexture(void);
	void createGraphicsPipeline(void);
	void createPresentGraphicsPipeline(void);
	void createFramebuffers(void);
	void createCommandPool(void);
	VkImageView createImageView(VkImage _image, VkFormat _format);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkCommandBuffer beginSingleTimeCommands(void);
	void endSingleTimeCommands(VkCommandBuffer);
	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void createUniformBuffers(void);
	void createVertexBuffers(void);
	void createIndexBuffer(void);
	void createDescriptorPool(void);
	void createDescriptorSets(void);
	void createCommandBuffers(void);
	void createSyncObjects(void);
	
	void recreateSwapChain(void);
	void recordCommandBuffer(VkCommandBuffer, uint32_t);

	void clearBuffers(void);
	void pushQuad(const sprite_t&, const position_t&, bool);
	void pushQuad(glm::vec2 _pos, glm::vec2 _size, glm::vec2 _texPos, glm::vec2 _texSize, uint32_t, bool);
	void pushText(glm::vec2 _pos, const std::string& _str);
	void pushLevel(const levelManager_t&);
	void pushGui(const gameManager_t&);
	////void drawLevel(const levelManager_t&);
	void updateUBO(glm::vec2);
	void updateRenderBuffers(void);
	void pushSprite(const sprite_t& _sprite, const position_t& _pos, bool _invertX, bool invertY);
	void drawFrame(void);

	void cleanupSwapChain(void);
	void cleanup(void);
};