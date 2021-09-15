#pragma once

#include "VkTypes.hpp"

#include <vector>

struct VkEngine {
	public:
		//General
		bool initialized{ false };
		int frameNumber{ 0 };

		VkExtent2D windowExtent{ 1700, 900 };

		struct SDL_Window* sdl_window{};

		void init();
		void deinit();

		void draw();
		void run();

	public:
		//Base Vulkan
		VkInstance vk_instance;
		VkDebugUtilsMessengerEXT vk_debug_messenger;
		VkPhysicalDevice vk_phys_dev;
		VkDevice vk_device;
		VkSurfaceKHR vk_surface;

		//Swapchain
		VkSwapchainKHR vk_swapchain;
		VkFormat vk_swapchain_format;
		std::vector<VkImage> vk_swapchain_imgs;
		std::vector<VkImageView> vk_swapchain_img_views;

		//Rendering
		VkQueue vk_graphics_queue;
		uint32_t vk_graphics_queue_family;

		VkCommandPool vk_cmd_pool;
		VkCommandBuffer main_cmd_buf;

		VkRenderPass vk_render_pass;
		std::vector<VkFramebuffer> vk_framebuffers;

	private:
		void init_vk();
		void init_vk_swapchain();
		void init_vk_cmd();

		void init_vk_default_renderpass();
		void init_vk_framebuffers();
};
