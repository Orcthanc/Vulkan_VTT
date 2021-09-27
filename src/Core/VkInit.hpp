#pragma once

#include "Core/VkTypes.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkinit {
	VkCommandPoolCreateInfo command_pool_create_info(
			uint32_t queue_family_index,
			VkCommandPoolCreateFlags = 0
		);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(
			VkCommandPool pool,
			uint32_t count = 1,
			VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
		);

	VkFenceCreateInfo fence_create_info(
			VkFenceCreateFlags flags = 0
		);

	VkSemaphoreCreateInfo semaphore_create_info(
			VkSemaphoreCreateFlags flags = 0
		);

	VkCommandBufferBeginInfo command_buffer_begin_info(
			VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			VkCommandBufferInheritanceInfo* inheritance_info = nullptr
		);

	VkPipelineShaderStageCreateInfo shader_stage_create_info(
			VkShaderStageFlagBits stage,
			VkShaderModule shader
		);

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(

		);

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(
			VkPrimitiveTopology topology
		);

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(
			VkPolygonMode polygon_mode
		);

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info(

		);

	VkPipelineColorBlendAttachmentState color_blend_attachment_state(

		);

	VkPipelineLayoutCreateInfo pipeline_layout(

		);

	VkImageCreateInfo image_create_info(
			VkFormat format,
			VkImageUsageFlags usage,
			VkExtent3D size
		);

	VkImageViewCreateInfo image_view_create_info(
			VkFormat format,
			VkImage image,
			VkImageAspectFlags aspect
		);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info(
			VkBool32 depth_test,
			VkBool32 depth_write,
			VkCompareOp op
		);

	VkSamplerCreateInfo sampler_create_info(
			VkFilter filter,
			VkSamplerAddressMode address_mode
		);

	VkWriteDescriptorSet write_descriptor_set_image(
			VkDescriptorType type,
			VkDescriptorSet dst_set,
			VkDescriptorImageInfo* img_info,
			uint32_t binding
		);
}
