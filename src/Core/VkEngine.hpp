#pragma once

#include "VkTypes.hpp"
#include "VkMesh.hpp"
#include "Camera/StrategyCam.hpp"

#include <vk_mem_alloc.h>

#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>
#include <string>
#include <vulkan/vulkan_core.h>

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
	VkDescriptorSet tex_set{ VK_NULL_HANDLE };
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct Texture {
	AllocatedImage img;
	VkImageView view;
};

struct RenderableObject {
	Mesh* mesh;
	Material* mat;
	glm::mat4 transform;
};

struct FrameData {
	VkSemaphore present_sema, render_sema;
	VkFence render_fence;

	VkCommandPool cmd_pool;
	VkCommandBuffer main_buf;

	AllocatedBuffer camera_buf;
	VkDescriptorSet global_desc;
};

struct GpuCamData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 view_proj;
};

struct UploadContext {
	VkFence fence;
	VkCommandPool cmd_pool;
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
		StrategyCamera cam;

		std::vector<RenderableObject> objects;

		std::unordered_map<std::string, Material> materials;
		std::unordered_map<std::string, Mesh> meshes;
		std::unordered_map<std::string, Texture> textures;

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

		UploadContext upload_context;

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

		constexpr static unsigned FRAME_OVERLAP = 2;
		FrameData frames[FRAME_OVERLAP];

		FrameData& get_curr_frame();

		VkRenderPass vk_render_pass;
		std::vector<VkFramebuffer> vk_framebuffers;

		VkDescriptorSetLayout global_desc_layout;
		VkDescriptorSetLayout single_tex_layout;
		VkDescriptorPool desc_pool;


	private:
		//Init
		void init_vk();
		void init_vk_swapchain();
		void init_vk_cmd();

		void init_vk_default_renderpass();
		void init_vk_framebuffers();

		void init_vk_sync();

		void init_vk_pipelines();

		void load_meshes();
		void load_images();

		void init_scene();

		void init_descriptors();

	public:
		//Vulkan helpers
		bool vk_load_shader( const char* path, VkShaderModule* shader );
		void upload_mesh( Mesh& mesh );

		void immediate_submit( std::function<void( VkCommandBuffer )>&& func );

		AllocatedBuffer create_buffer( size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage );
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
