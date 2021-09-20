#pragma once

#include "VkTypes.hpp"
#include "VkMesh.hpp"

#include <vk_mem_alloc.h>

#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>
#include <string>

struct DelQueue {
	std::deque<std::function<void()>> deleters;

	inline void emplace_function( std::function<void()>&& func ){
		deleters.emplace_back( std::move( func ));
	}

	inline void flush(){
		for( auto it = deleters.rbegin(); it != deleters.rend(); ++it ){
			(*it)();
		}

		deleters.clear();
	}
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct RenderableObject {
	Mesh* mesh;
	Material* mat;
	glm::mat4 transform;
};

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
		//Scene
		std::vector<RenderableObject> objects;

		std::unordered_map<std::string, Material> materials;
		std::unordered_map<std::string, Mesh> meshes;

		Material* create_material( VkPipeline pipeline, VkPipelineLayout layout, const std::string& name );
		Material* get_material( const std::string& name );

		Mesh* get_mesh( const std::string& name );

		void draw_objects( VkCommandBuffer cmd, RenderableObject* first, int count );

	public:
		//Base Vulkan
		VkInstance vk_instance;
		VkDebugUtilsMessengerEXT vk_debug_messenger;
		VkPhysicalDevice vk_phys_dev;
		VkDevice vk_device;
		VkSurfaceKHR vk_surface;

		DelQueue deletion_queue;
		VmaAllocator vma_alloc;

		//Swapchain
		VkSwapchainKHR vk_swapchain;
		VkFormat vk_swapchain_format;
		std::vector<VkImage> vk_swapchain_imgs;
		std::vector<VkImageView> vk_swapchain_img_views;

		VkImageView depth_view;
		AllocatedImage depth_img;

		VkFormat depth_format;

		//Rendering
		VkQueue vk_graphics_queue;
		uint32_t vk_graphics_queue_family;

		VkCommandPool vk_cmd_pool;
		VkCommandBuffer main_cmd_buf;

		VkRenderPass vk_render_pass;
		std::vector<VkFramebuffer> vk_framebuffers;

		//Synchronization
		VkSemaphore vk_sema_present, vk_sema_render;
		VkFence vk_fence_render;

	private:
		void init_vk();
		void init_vk_swapchain();
		void init_vk_cmd();

		void init_vk_default_renderpass();
		void init_vk_framebuffers();

		void init_vk_sync();

		bool vk_load_shader( const char* path, VkShaderModule* shader );
		void init_vk_pipelines();

		void load_meshes();

		void upload_mesh( Mesh& mesh );

		void init_scene();
};

struct PipelineBuilder {
	VkPipeline build_pipeline( VkDevice dev, VkRenderPass pass );

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
	VkPipelineVertexInputStateCreateInfo vertex_in_info;
	VkPipelineInputAssemblyStateCreateInfo input_assembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState color_blend;
	VkPipelineMultisampleStateCreateInfo multisample_state;
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
	VkPipelineLayout pipeline_layout;
};
