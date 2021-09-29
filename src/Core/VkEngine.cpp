#include "Core/VkEngine.hpp"
#include "Core/VkMesh.hpp"
#include "Core/VkTypes.hpp"
#include "Core/VkTexture.hpp"
#include <SDL_keyboard.h>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/gtx/transform.hpp>

#include <cstdint>
#include <fstream>
#include <ios>
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


#ifdef NO_FILE_PREFIX
	#define FILE_PREFIX
#else
	#if _WIN32
		#define FILE_PREFIX "../../../../"
	#else
		#define FILE_PREFIX "../../"
	#endif
#endif


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
	init_vk_sync();

	init_descriptors();

	init_vk_pipelines();

	load_meshes();
	load_images();

	init_scene();

	initialized = true;
}

void VkEngine::deinit(){
	if( initialized ){
		//Vulkan
		vkDeviceWaitIdle( vk_device );

		/*
		vkDestroyFence( vk_device, vk_fence_render, nullptr );
		vkDestroySemaphore( vk_device, vk_sema_render, nullptr );
		vkDestroySemaphore( vk_device, vk_sema_present, nullptr );

		vkDestroyCommandPool( vk_device, vk_cmd_pool, nullptr );

		vkDestroySwapchainKHR( vk_device, vk_swapchain, nullptr );

		vkDestroyRenderPass( vk_device, vk_render_pass, nullptr );

		for( size_t i = 0; i < vk_swapchain_img_views.size(); ++i ){
			vkDestroyFramebuffer( vk_device, vk_framebuffers[i], nullptr );
			vkDestroyImageView( vk_device, vk_swapchain_img_views[i], nullptr );
		}
		*/

		deletion_queue.flush();

		vmaDestroyAllocator( vma_alloc );

		vkDestroyDevice( vk_device, nullptr );
		vkDestroySurfaceKHR( vk_instance, vk_surface, nullptr );
		vkb::destroy_debug_utils_messenger( vk_instance, vk_debug_messenger );
		vkDestroyInstance( vk_instance, nullptr );

		//SDL
		SDL_DestroyWindow( sdl_window );

		SDL_Quit();
	}
	initialized = false;
}

void VkEngine::draw(){
	VK_CHECK( vkWaitForFences( vk_device, 1, &get_curr_frame().render_fence, VK_TRUE, 1000000000 ));
	VK_CHECK( vkResetFences( vk_device, 1, &get_curr_frame().render_fence ));

	uint32_t render_img;
	VK_CHECK( vkAcquireNextImageKHR( vk_device, vk_swapchain, 1000000000, get_curr_frame().present_sema, VK_NULL_HANDLE, &render_img ));

	VK_CHECK( vkResetCommandBuffer( get_curr_frame().main_buf, 0 ));
	auto beg_inf = vkinit::command_buffer_begin_info();

	VK_CHECK( vkBeginCommandBuffer( get_curr_frame().main_buf, &beg_inf ));

	VkClearValue clear_vals[2]{
		{
			.color = {{ 0.1, 0.1, 0.1, 1 }},
		},
		{
			.depthStencil = {
				.depth = 1.0f,
			}
		}
	};

	VkRenderPassBeginInfo render_beg_inf{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = vk_render_pass,
		.framebuffer = vk_framebuffers[render_img],
		.renderArea = VkRect2D{
			.offset = { 0, 0 },
			.extent = windowExtent,
		},
		.clearValueCount = 2,
		.pClearValues = clear_vals,
	};

	vkCmdBeginRenderPass( get_curr_frame().main_buf, &render_beg_inf, VK_SUBPASS_CONTENTS_INLINE );

	draw_objects( get_curr_frame().main_buf, objects.data(), objects.size() );

	vkCmdEndRenderPass( get_curr_frame().main_buf );
	VK_CHECK( vkEndCommandBuffer( get_curr_frame().main_buf ));

	VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo sub_inf {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &get_curr_frame().present_sema,
		.pWaitDstStageMask = &waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &get_curr_frame().main_buf,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &get_curr_frame().render_sema,
	};

	VK_CHECK( vkQueueSubmit( vk_graphics_queue, 1, &sub_inf, get_curr_frame().render_fence ));

	VkPresentInfoKHR pres_inf = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &get_curr_frame().render_sema,
		.swapchainCount = 1,
		.pSwapchains = &vk_swapchain,
		.pImageIndices = &render_img,
	};

	VK_CHECK( vkQueuePresentKHR( vk_graphics_queue,  &pres_inf ));

	++frameNumber;
}

void VkEngine::run(){
	SDL_Event e;
	bool quit = false;

	float dT = 0.013;

	while( !quit ){
		while( SDL_PollEvent( &e )){
			if( e.type == SDL_QUIT )
				quit = true;
			switch( e.type ){
				case SDL_QUIT:
				{
					quit = true;
					break;
				}
				case SDL_MOUSEWHEEL:
				{
					cam.move_from_anchor({ 0.0f, e.wheel.y * dT * 100 });
					break;
				}
			}
		}
		
		// Keys
		{
			const Uint8* state = SDL_GetKeyboardState( nullptr );

			float rotate{};
			glm::vec3 move{};

			if( state[SDL_SCANCODE_Q] ){
				rotate -= 1 * dT;
			}
			if( state[SDL_SCANCODE_E] ){
				rotate += 1 * dT;
			}
			if( state[SDL_SCANCODE_W] ){
				move.x += 10 * dT;
			}
			if( state[SDL_SCANCODE_S] ){
				move.x -= 10 * dT;
			}
			if( state[SDL_SCANCODE_D] ){
				move.z += 10 * dT;
			}
			if( state[SDL_SCANCODE_A] ){
				move.z -= 10 * dT;
			}

			cam.rotate_around_origin( rotate );
			cam.move_anchor( move );
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

	VmaAllocatorCreateInfo alloc_inf{
		.physicalDevice = vk_phys_dev,
		.device = vk_device,
		.instance = vk_instance,
	};

	vmaCreateAllocator( &alloc_inf, &vma_alloc );
}

void VkEngine::init_vk_swapchain(){
	vkb::SwapchainBuilder swapchain_builder{ vk_phys_dev, vk_device, vk_surface };
	vkb::Swapchain vkb_swapchain = swapchain_builder
		.use_default_format_selection()
//		.set_desired_present_mode( VK_PRESENT_MODE_MAILBOX_KHR )
		.set_desired_present_mode( VK_PRESENT_MODE_FIFO_RELAXED_KHR )
		.add_fallback_present_mode( VK_PRESENT_MODE_FIFO_KHR )
		.set_desired_extent( windowExtent.width, windowExtent.height )
		.build()
		.value();

	vk_swapchain = vkb_swapchain.swapchain;
	vk_swapchain_format = vkb_swapchain.image_format;
	vk_swapchain_imgs = vkb_swapchain.get_images().value();
	vk_swapchain_img_views = vkb_swapchain.get_image_views().value();

	deletion_queue.emplace_function( [this](){
			for( size_t i = 0; i < vk_swapchain_img_views.size(); ++i ){
				vkDestroyImageView( vk_device, vk_swapchain_img_views[i], nullptr );
			}
		});

	deletion_queue.emplace_function( [this](){ vkDestroySwapchainKHR( vk_device, vk_swapchain, nullptr ); });

	VkExtent3D depth_img_size = {
		.width = windowExtent.width,
		.height = windowExtent.height,
		.depth = 1,
	};

	depth_format = VK_FORMAT_D32_SFLOAT;

	auto img_cr_inf = vkinit::image_create_info( depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_img_size );

	VmaAllocationCreateInfo img_alloc_inf = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	vmaCreateImage( vma_alloc, &img_cr_inf, &img_alloc_inf, &depth_img.image, &depth_img.allocation, nullptr );

	VkImageViewCreateInfo view_cr_inf = vkinit::image_view_create_info( depth_format, depth_img.image, VK_IMAGE_ASPECT_DEPTH_BIT );

	VK_CHECK( vkCreateImageView( vk_device, &view_cr_inf, nullptr, &depth_view ));

	deletion_queue.emplace_function( [this](){
			vkDestroyImageView( vk_device, depth_view, nullptr );
			vmaDestroyImage( vma_alloc, depth_img.image, depth_img.allocation );
		});
}

void VkEngine::init_vk_cmd(){
	auto cmd_pool_cr_inf =
		vkinit::command_pool_create_info(
			vk_graphics_queue_family,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		);

	for( size_t i = 0; i < FRAME_OVERLAP; ++i ){
		VK_CHECK( vkCreateCommandPool( vk_device, &cmd_pool_cr_inf, nullptr, &frames[i].cmd_pool ));

		deletion_queue.emplace_function( [this, i](){ vkDestroyCommandPool( vk_device, frames[i].cmd_pool, nullptr ); });

		auto cmd_alloc_inf =
			vkinit::command_buffer_allocate_info(
				frames[i].cmd_pool
			);

		VK_CHECK( vkAllocateCommandBuffers( vk_device, &cmd_alloc_inf, &frames[i].main_buf ));
	}

	auto up_cmd_pl_inf = vkinit::command_pool_create_info( vk_graphics_queue_family );
	VK_CHECK( vkCreateCommandPool( vk_device, &up_cmd_pl_inf, nullptr, &upload_context.cmd_pool ));

	deletion_queue.emplace_function( [this](){
			vkDestroyCommandPool( vk_device, upload_context.cmd_pool, nullptr );
		});
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

	VkAttachmentDescription depth_attachment {
		.format = depth_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attach_ref {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attach_ref,
		.pDepthStencilAttachment = &depth_attach_ref,
	};

	VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_cr_inf {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	VK_CHECK( vkCreateRenderPass( vk_device, &render_pass_cr_inf, nullptr, &vk_render_pass ));

	deletion_queue.emplace_function( [this](){ vkDestroyRenderPass( vk_device, vk_render_pass, nullptr ); });
}

void VkEngine::init_vk_framebuffers(){
	VkFramebufferCreateInfo frame_cr_inf{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.renderPass = vk_render_pass,
		.attachmentCount = 2,
		.width = windowExtent.width,
		.height = windowExtent.height,
		.layers = 1,
	};

	const uint32_t swap_size = vk_swapchain_imgs.size();
	vk_framebuffers.resize( swap_size );

	for( size_t i = 0; i < swap_size; ++i ){
		VkImageView attachments[2];
		attachments[0] = vk_swapchain_img_views[i];
		attachments[1] = depth_view;

		frame_cr_inf.pAttachments = attachments;
		VK_CHECK( vkCreateFramebuffer( vk_device, &frame_cr_inf, nullptr, &vk_framebuffers[i] ));

		deletion_queue.emplace_function( [this, i](){ vkDestroyFramebuffer( vk_device, vk_framebuffers[i], nullptr); });
	}
}

void VkEngine::init_vk_sync(){
	auto fence_cr_inf = vkinit::fence_create_info( 0 );
	auto sem_cr_inf = vkinit::semaphore_create_info();

	VK_CHECK( vkCreateFence( vk_device, &fence_cr_inf, nullptr, &upload_context.fence ));
	deletion_queue.emplace_function( [this](){
			vkDestroyFence( vk_device, upload_context.fence, nullptr );
		});

	fence_cr_inf.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for( size_t i = 0; i < FRAME_OVERLAP; ++i ){
		VK_CHECK( vkCreateFence( vk_device, &fence_cr_inf, nullptr, &frames[i].render_fence ));

		deletion_queue.emplace_function( [this, i](){ vkDestroyFence( vk_device, frames[i].render_fence, nullptr ); });

		VK_CHECK( vkCreateSemaphore( vk_device, &sem_cr_inf, nullptr, &frames[i].render_sema ));
		VK_CHECK( vkCreateSemaphore( vk_device, &sem_cr_inf, nullptr, &frames[i].present_sema ));

		deletion_queue.emplace_function( [this, i](){ vkDestroySemaphore( vk_device, frames[i].render_sema, nullptr ); });
		deletion_queue.emplace_function( [this, i](){ vkDestroySemaphore( vk_device, frames[i].present_sema, nullptr ); });
	}

}

bool VkEngine::vk_load_shader( const char* path, VkShaderModule* shader ){
	std::ifstream file( path, std::ios::ate | std::ios::binary );

	if( !file.is_open()){
		std::cout << "Could not open file " << path << std::endl;
		return false;
	}

	auto size = file.tellg();
	file.seekg( 0 );

	std::vector<uint32_t> buffer( size / 4 );
	file.read( reinterpret_cast<char*>( buffer.data() ), size );
	file.close();

	VkShaderModuleCreateInfo shader_cr_inf{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = static_cast<uint32_t>( size ),
		.pCode = buffer.data(),
	};

	if( vkCreateShaderModule( vk_device, &shader_cr_inf, nullptr, shader )){
		return false;
	}
	return true;
}

void VkEngine::init_vk_pipelines(){
	VkShaderModule triVert{}, triFrag{};


	if (!vk_load_shader(FILE_PREFIX "shader/triangle.vert.spv", &triVert)) {
		std::cout << "Failed to load vert shader" << std::endl;
	}

	if (!vk_load_shader(FILE_PREFIX "shader/triangle.frag.spv", &triFrag)) {
		std::cout << "Failed to load vert shader" << std::endl;
	}

	auto pipe_lay_cr_inf = vkinit::pipeline_layout();

	VkPushConstantRange push_constant{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof( PushConstants ),
	};

	VkDescriptorSetLayout layouts[2] = { global_desc_layout, single_tex_layout };

	pipe_lay_cr_inf.pushConstantRangeCount = 1;
	pipe_lay_cr_inf.pPushConstantRanges = &push_constant;
	pipe_lay_cr_inf.setLayoutCount = 2;
	pipe_lay_cr_inf.pSetLayouts = layouts;

	VkPipelineLayout triangle_layout;

	VK_CHECK( vkCreatePipelineLayout( vk_device, &pipe_lay_cr_inf, nullptr, &triangle_layout ));

	deletion_queue.emplace_function( [this, triangle_layout](){ vkDestroyPipelineLayout( vk_device, triangle_layout, nullptr ); });

	PipelineBuilder pipe_builder;


	VertexInputDescription vertex_desc{ Vertex::get_vk_description() };

	pipe_builder.vertex_in_info = vkinit::vertex_input_state_create_info();
	pipe_builder.vertex_in_info.vertexAttributeDescriptionCount = vertex_desc.attributes.size();
	pipe_builder.vertex_in_info.pVertexAttributeDescriptions = vertex_desc.attributes.data();

	pipe_builder.vertex_in_info.vertexBindingDescriptionCount = vertex_desc.bindings.size();
	pipe_builder.vertex_in_info.pVertexBindingDescriptions = vertex_desc.bindings.data();


	pipe_builder.shader_stages.push_back(
			vkinit::shader_stage_create_info( VK_SHADER_STAGE_VERTEX_BIT, triVert ));

	pipe_builder.shader_stages.push_back(
			vkinit::shader_stage_create_info( VK_SHADER_STAGE_FRAGMENT_BIT, triFrag ));


	pipe_builder.input_assembly = vkinit::input_assembly_state_create_info( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );

	pipe_builder.viewport.x = 0;
	pipe_builder.viewport.y = 0;
	pipe_builder.viewport.width = windowExtent.width;
	pipe_builder.viewport.height = windowExtent.height;
	pipe_builder.viewport.minDepth = 0;
	pipe_builder.viewport.maxDepth = 1;

	pipe_builder.scissor.offset = { 0, 0 };
	pipe_builder.scissor.extent = windowExtent;

	pipe_builder.rasterizer = vkinit::rasterization_state_create_info( VK_POLYGON_MODE_FILL );
	pipe_builder.multisample_state = vkinit::multisample_state_create_info();
	pipe_builder.color_blend = vkinit::color_blend_attachment_state();
	pipe_builder.depth_stencil_state = vkinit::depth_stencil_state_create_info( VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL );
	pipe_builder.pipeline_layout = triangle_layout;

	VkPipeline triangle_pipeline;

	triangle_pipeline = pipe_builder.build_pipeline( vk_device, vk_render_pass );

	vkDestroyShaderModule( vk_device, triVert, nullptr );
	vkDestroyShaderModule( vk_device, triFrag, nullptr );

	deletion_queue.emplace_function( [this, triangle_pipeline](){ vkDestroyPipeline( vk_device, triangle_pipeline, nullptr); });

	create_material( triangle_pipeline, triangle_layout, "default" );
}

VkPipeline PipelineBuilder::build_pipeline( VkDevice dev, VkRenderPass pass ){
	VkPipelineViewportStateCreateInfo view_state_cr_inf{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	VkPipelineColorBlendStateCreateInfo color_blend_cr_inf{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend,
	};

	VkGraphicsPipelineCreateInfo pipe_cr_inf{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.stageCount = static_cast<uint32_t>( shader_stages.size() ),
		.pStages = shader_stages.data(),
		.pVertexInputState = &vertex_in_info,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &view_state_cr_inf,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisample_state,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blend_cr_inf,
		.layout = pipeline_layout,
		.renderPass = pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
	};

	VkPipeline pipe;

	if( VK_SUCCESS != vkCreateGraphicsPipelines( dev, VK_NULL_HANDLE, 1, &pipe_cr_inf, nullptr, &pipe )){
		std::cout << "Could not create pipeline" << std::endl;
		return VK_NULL_HANDLE;
	}
	return pipe;
}

void VkEngine::load_meshes(){
	Mesh triangle_mesh;

	//make the array 3 vertices long
	triangle_mesh.vertices.resize(3);

	//vertex poss
	triangle_mesh.vertices[0].pos = { 1.0f, 1.0f, 0.0f };
	triangle_mesh.vertices[1].pos = {-1.0f, 1.0f, 0.0f };
	triangle_mesh.vertices[2].pos = { 0.0f,-1.0f, 0.0f };

	//vertex colors, all green
	triangle_mesh.vertices[0].color = { 0.0f, 1.0f, 0.0f }; //pure green
	triangle_mesh.vertices[1].color = { 0.0f, 1.0f, 0.0f }; //pure green
	triangle_mesh.vertices[2].color = { 0.0f, 1.0f, 0.0f }; //pure green

	//we don't care about the vertex normals

	upload_mesh(triangle_mesh);

	meshes["triangle"] = triangle_mesh;

	Mesh plate;
	plate.vertices.resize( 6 );

	plate.vertices[0] = Vertex{
		.pos =     { 0.5f, 0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 1.0f, 1.0f, 0.0f, 0.0f },
	};
	plate.vertices[1] = Vertex{
		.pos =     {-0.5f, 0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 0.0f, 1.0f, 0.0f, 0.0f },
	};
	plate.vertices[2] = Vertex{
		.pos =     {-0.5f,-0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 0.0f, 0.0f, 0.0f, 0.0f },
	};
	plate.vertices[3] = Vertex{
		.pos =     {-0.5f,-0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 0.0f, 0.0f, 0.0f, 0.0f },
	};
	plate.vertices[4] = Vertex{
		.pos =     { 0.5f,-0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 1.0f, 0.0f, 0.0f, 0.0f },
	};
	plate.vertices[5] = Vertex{
		.pos =     { 0.5f, 0.5f, 0.0f },
		.normal =  { 0.0f, 0.0f,-1.0f },
		.color =   { 1.0f, 1.0f, 1.0f },
		.uv1_uv2 = { 1.0f, 1.0f, 0.0f, 0.0f },
	};

	upload_mesh( plate );

	meshes["plane"] = plate;
}

Material* VkEngine::create_material( VkPipeline pipeline, VkPipelineLayout layout, const std::string& name ){
	Material mat{
		.pipeline = pipeline,
		.layout = layout,
	};
	materials[name] = mat;
	return &materials[name];
}

Material* VkEngine::get_material( const std::string& name ){
	auto it = materials.find( name );
	if( it == materials.end() )
		return nullptr;
	else
		return &it->second;
}

Mesh* VkEngine::get_mesh( const std::string& name ){
	auto it = meshes.find( name );
	if( it == meshes.end() )
		return nullptr;
	else
		return &it->second;
}

void VkEngine::draw_objects( VkCommandBuffer cmd, RenderableObject* first, int count ){

	//cam.rotate_around_origin( 0.02 );

	glm::mat4 proj = cam.get_proj();
	glm::mat4 view = cam.get_view();

	GpuCamData cam_data{
		.view = view,
		.proj = proj,
		.view_proj = proj * view,
	};

	void* data;
	vmaMapMemory( vma_alloc, get_curr_frame().camera_buf.allocation, &data );
	memcpy( data, &cam_data, sizeof( GpuCamData ));
	vmaUnmapMemory( vma_alloc, get_curr_frame().camera_buf.allocation );

	Mesh* last_mesh = nullptr;
	Material* last_mat = nullptr;

	for( size_t i = 0; i < count; ++i ){
		RenderableObject& curr = first[i];

		if( curr.mat != last_mat ){
			vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, curr.mat->pipeline );
			last_mat = curr.mat;

			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, curr.mat->layout, 0, 1, &get_curr_frame().global_desc, 0, nullptr );

			if( curr.mat->tex_set ){
				vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, curr.mat->layout, 1, 1, &curr.mat->tex_set, 0, nullptr );
			}
		}

		PushConstants consts{
			.camera = curr.transform /* * glm::rotate( frameNumber * 0.04f, glm::vec3{ 0.0f, 1.0f, 0.0f }) */,
		};

		vkCmdPushConstants( cmd, curr.mat->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PushConstants ), &consts );

		if( curr.mesh != last_mesh ){
			VkDeviceSize off = 0;
			vkCmdBindVertexBuffers( cmd, 0, 1, &curr.mesh->buffer.buffer, &off );
			last_mesh = curr.mesh;
		}

		vkCmdDraw( cmd, curr.mesh->vertices.size(), 1, 0, 0 );
	}
}

void VkEngine::upload_mesh( Mesh& mesh ){
	VkBufferCreateInfo buf_cr_inf{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = mesh.vertices.size() * sizeof( Vertex ),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	};

	VmaAllocationCreateInfo vma_alloc_inf {
		.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
	};

	VK_CHECK( vmaCreateBuffer( vma_alloc, &buf_cr_inf, &vma_alloc_inf, &mesh.buffer.buffer, &mesh.buffer.allocation, nullptr ));

	deletion_queue.emplace_function( [this, mesh](){ vmaDestroyBuffer( vma_alloc, mesh.buffer.buffer, mesh.buffer.allocation ); });

	void* data;

	vmaMapMemory( vma_alloc, mesh.buffer.allocation, &data );
	memcpy( data, mesh.vertices.data(), mesh.vertices.size() * sizeof( Vertex ));
	vmaUnmapMemory( vma_alloc, mesh.buffer.allocation );
}

void VkEngine::init_scene(){
	glm::mat4 proj = glm::perspective( static_cast<float>( 0.25 * M_PI ), static_cast<float>( windowExtent.width ) / static_cast<float>( windowExtent.height ), 0.01f, 200.0f );
	proj[1][1] *= -1;
	cam.set_proj( proj );


	RenderableObject tri{
		.mesh = get_mesh( "plane" ),
		.mat = get_material( "default" ),
		.transform = glm::mat4( 1.0f ),
	};

	auto sampler_inf = vkinit::sampler_create_info( VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT );

	VkSampler block_sampler;
	vkCreateSampler( vk_device, &sampler_inf, nullptr, &block_sampler );

	deletion_queue.emplace_function( [this, block_sampler](){
			vkDestroySampler( vk_device, block_sampler, nullptr );
		});

	VkDescriptorSetAllocateInfo alloc_inf{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &single_tex_layout,
	};

	vkAllocateDescriptorSets( vk_device, &alloc_inf, &tri.mat->tex_set );

	VkDescriptorImageInfo img_inf{
		.sampler = block_sampler,
		.imageView = textures["outline"].view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	auto tex1_write = vkinit::write_descriptor_set_image( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, tri.mat->tex_set, &img_inf, 0 );

	vkUpdateDescriptorSets( vk_device, 1, &tex1_write, 0, nullptr );

	for( int y = 0; y < 21; ++y ){
		for( int x = 0; x < 21; ++x ){
			tri.transform = glm::translate( glm::vec3{ x - 10.0f, 0, y - 10.0f }) * glm::rotate<float>( 0.5 * M_PI, glm::vec3{ 1.0f, 0.0f, 0.0f });
			objects.push_back( tri );
		}
	}
}

FrameData& VkEngine::get_curr_frame(){
	return frames[frameNumber % FRAME_OVERLAP];
}

AllocatedBuffer VkEngine::create_buffer( size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage ){
	VkBufferCreateInfo buf_inf{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = size,
		.usage = usage,
	};

	VmaAllocationCreateInfo vma_alloc_inf{
		.usage = memory_usage,
	};

	AllocatedBuffer buf;

	VK_CHECK( vmaCreateBuffer( vma_alloc, &buf_inf, &vma_alloc_inf, &buf.buffer, &buf.allocation, nullptr ));

	return buf;
}

void VkEngine::init_descriptors(){

	VkDescriptorSetLayoutBinding binding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
	};

	VkDescriptorSetLayoutCreateInfo desc_set_lay_cr_inf{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = 1,
		.pBindings = &binding,
	};

	VkDescriptorSetLayoutBinding binding_tex {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	VkDescriptorSetLayoutCreateInfo desc_set_tex_cr_inf {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.bindingCount = 1,
		.pBindings = &binding_tex,
	};

	vkCreateDescriptorSetLayout( vk_device, &desc_set_lay_cr_inf, nullptr, &global_desc_layout );
	vkCreateDescriptorSetLayout( vk_device, &desc_set_tex_cr_inf, nullptr, &single_tex_layout );

	deletion_queue.emplace_function( [this](){
			vkDestroyDescriptorSetLayout( vk_device, single_tex_layout, nullptr );
			vkDestroyDescriptorSetLayout( vk_device, global_desc_layout, nullptr );
		});

	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
	};

	VkDescriptorPoolCreateInfo desc_pool_cr_inf{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = 10,
		.poolSizeCount = static_cast<uint32_t>( sizes.size() ),
		.pPoolSizes = sizes.data(),
	};

	vkCreateDescriptorPool( vk_device, &desc_pool_cr_inf, nullptr, &desc_pool );

	deletion_queue.emplace_function( [this](){ vkDestroyDescriptorPool( vk_device, desc_pool, nullptr ); });


	for( size_t i = 0; i < FRAME_OVERLAP; ++i ){
		frames[i].camera_buf = create_buffer( sizeof( GpuCamData ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU );

		deletion_queue.emplace_function( [this, i](){
				vmaDestroyBuffer( vma_alloc, frames[i].camera_buf.buffer, frames[i].camera_buf.allocation );
			});

		VkDescriptorSetAllocateInfo alloc_inf{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = desc_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &global_desc_layout,
		};

		vkAllocateDescriptorSets( vk_device, &alloc_inf, &frames[i].global_desc );

		VkDescriptorBufferInfo buf_inf{
			.buffer = frames[i].camera_buf.buffer,
			.offset = 0,
			.range = sizeof( GpuCamData ),
		};

		VkWriteDescriptorSet set_write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = frames[i].global_desc,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &buf_inf,
		};

		vkUpdateDescriptorSets( vk_device, 1, &set_write, 0, nullptr );
	}
}

void VkEngine::immediate_submit( std::function<void( VkCommandBuffer )>&& func ){
	VkCommandBufferAllocateInfo cmd_alloc = vkinit::command_buffer_allocate_info( upload_context.cmd_pool );

	VkCommandBuffer buf;
	VK_CHECK( vkAllocateCommandBuffers(vk_device, &cmd_alloc, &buf ));

	VkCommandBufferBeginInfo cmd_beg = vkinit::command_buffer_begin_info( VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );

	VK_CHECK( vkBeginCommandBuffer( buf, &cmd_beg ));

	func( buf );

	VK_CHECK( vkEndCommandBuffer( buf ));

	VkSubmitInfo sub_inf{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &buf,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
	};

	VK_CHECK( vkQueueSubmit( vk_graphics_queue, 1, &sub_inf, upload_context.fence ));

	vkWaitForFences( vk_device, 1, &upload_context.fence, VK_TRUE, 999999999999 );
	vkResetFences( vk_device, 1, &upload_context.fence );

	vkResetCommandPool( vk_device, upload_context.cmd_pool, 0 );
}

void VkEngine::load_images(){
	Texture outline;

	vkutil::load_image_file( *this, FILE_PREFIX "assets/outline.png", outline.img );

	auto view_cr = vkinit::image_view_create_info( VK_FORMAT_R8G8B8A8_SRGB, outline.img.image, VK_IMAGE_ASPECT_COLOR_BIT );
	VK_CHECK( vkCreateImageView( vk_device, &view_cr, nullptr, &outline.view ));
	deletion_queue.emplace_function( [this, outline](){
			vkDestroyImageView( vk_device, outline.view, nullptr );
		});

	textures["outline"] = outline;
}
