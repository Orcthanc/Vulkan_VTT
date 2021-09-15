#include "Core/VkEngine.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "Core/VkInit.hpp"
#include "VkBootstrap.h"

#define VK_CHECK( x ) 											\
	do { 														\
		VkResult err = x; 										\
		if( err ){ 												\
			std::cout << "Vulkan error: " << err << std::endl; 	\
			throw std::runtime_error( "Vulkan error" ); 		\
		} 														\
	}while( 0 )

void VkEngine::init(){
	SDL_Init( SDL_INIT_VIDEO );

	SDL_WindowFlags window_flags{ SDL_WINDOW_VULKAN };

	sdl_window = SDL_CreateWindow(
			"VTableTop",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			windowExtent.width,
			windowExtent.height,
			window_flags
		);

	init_vk();
	init_vk_swapchain();
	init_vk_cmd();
	init_vk_default_renderpass();
	init_vk_framebuffers();

	initialized = true;
}

void VkEngine::deinit(){
	if( initialized ){
		//Vulkan
		vkDestroyCommandPool( vk_device, vk_cmd_pool, nullptr );

		vkDestroySwapchainKHR( vk_device, vk_swapchain, nullptr );

		vkDestroyRenderPass( vk_device, vk_render_pass, nullptr );

		for( size_t i = 0; i < vk_swapchain_img_views.size(); ++i ){
			vkDestroyFramebuffer( vk_device, vk_framebuffers[i], nullptr );
			vkDestroyImageView( vk_device, vk_swapchain_img_views[i], nullptr );
		}

		vkDestroyDevice( vk_device, nullptr );
		vkDestroySurfaceKHR( vk_instance, vk_surface, nullptr );
		vkb::destroy_debug_utils_messenger( vk_instance, vk_debug_messenger );
		vkDestroyInstance( vk_instance, nullptr );

		//SDL
		SDL_DestroyWindow( sdl_window );
	}
	initialized = false;
}

void VkEngine::draw(){

}

void VkEngine::run(){
	SDL_Event e;
	bool quit = false;

	while( !quit ){
		while( SDL_PollEvent( &e )){
			if( e.type == SDL_QUIT )
				quit = true;
		}

		draw();
	}
}

void VkEngine::init_vk(){
	//Instance
	vkb::InstanceBuilder builder;

	//TODO logging and validation layer switch
	auto inst_ret = builder
		.set_app_name( "VTT" )
		.request_validation_layers( true )
		.require_api_version( 1, 2 )
		.use_default_debug_messenger()
		.build();

	auto vkb_inst = inst_ret.value();
	vk_instance = vkb_inst.instance;
	vk_debug_messenger = vkb_inst.debug_messenger;

	//Surface
	SDL_Vulkan_CreateSurface( sdl_window, vk_instance, &vk_surface );

	//Physical Device
	vkb::PhysicalDeviceSelector phys_sel{ vkb_inst };
	vkb::PhysicalDevice vkb_phys_dev = phys_sel
		.set_minimum_version( 1, 2 )
		.set_surface( vk_surface )
		.prefer_gpu_device_type()
		.select()
		.value();

	vk_phys_dev = vkb_phys_dev.physical_device;

	//Logical Device
	vkb::DeviceBuilder device_builder{ vkb_phys_dev };

	vkb::Device vkb_device = device_builder
		.build()
		.value();

	vk_device = vkb_device.device;
	vk_graphics_queue = vkb_device.get_queue( vkb::QueueType::graphics ).value();
	vk_graphics_queue_family = vkb_device.get_queue_index( vkb::QueueType::graphics ).value();
}

void VkEngine::init_vk_swapchain(){
	vkb::SwapchainBuilder swapchain_builder{ vk_phys_dev, vk_device, vk_surface };
	vkb::Swapchain vkb_swapchain = swapchain_builder
		.use_default_format_selection()
		.set_desired_present_mode( VK_PRESENT_MODE_MAILBOX_KHR )
		.add_fallback_present_mode( VK_PRESENT_MODE_FIFO_KHR )
		.set_desired_extent( windowExtent.width, windowExtent.height )
		.build()
		.value();

	vk_swapchain = vkb_swapchain.swapchain;
	vk_swapchain_format = vkb_swapchain.image_format;
	vk_swapchain_imgs = vkb_swapchain.get_images().value();
	vk_swapchain_img_views = vkb_swapchain.get_image_views().value();
}

void VkEngine::init_vk_cmd(){
	auto cmd_pool_cr_inf =
		vkinit::command_pool_create_info(
			vk_graphics_queue_family,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		);

	VK_CHECK( vkCreateCommandPool( vk_device, &cmd_pool_cr_inf, nullptr, &vk_cmd_pool ));

	auto cmd_alloc_inf =
		vkinit::command_buffer_allocate_info(
			vk_cmd_pool
		);

	VK_CHECK( vkAllocateCommandBuffers( vk_device, &cmd_alloc_inf, &main_cmd_buf ));
}

void VkEngine::init_vk_default_renderpass(){
	VkAttachmentDescription color_attachment{
		.format = vk_swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference color_attach_ref {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attach_ref,
	};

	VkRenderPassCreateInfo render_pass_cr_inf {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	VK_CHECK( vkCreateRenderPass( vk_device, &render_pass_cr_inf, nullptr, &vk_render_pass ));
}

void VkEngine::init_vk_framebuffers(){
	VkFramebufferCreateInfo frame_cr_inf{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.renderPass = vk_render_pass,
		.attachmentCount = 1,
		.width = windowExtent.width,
		.height = windowExtent.height,
		.layers = 1,
	};

	const uint32_t swap_size = vk_swapchain_imgs.size();
	vk_framebuffers.resize( swap_size );

	for( size_t i = 0; i < swap_size; ++i ){
		frame_cr_inf.pAttachments = &vk_swapchain_img_views[i];
		VK_CHECK( vkCreateFramebuffer( vk_device, &frame_cr_inf, nullptr, &vk_framebuffers[i] ));
	}
}
