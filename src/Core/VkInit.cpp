#include "Core/VkInit.hpp"
#include <vulkan/vulkan_core.h>

VkCommandPoolCreateInfo vkinit::command_pool_create_info(
		uint32_t queue_family_index,
		VkCommandPoolCreateFlags flags ){

	return VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.queueFamilyIndex = queue_family_index,
	};
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(
		VkCommandPool pool,
		uint32_t count,
		VkCommandBufferLevel level ){

	return VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = pool,
		.level = level,
		.commandBufferCount = count,
	};
}


VkFenceCreateInfo vkinit::fence_create_info(
		VkFenceCreateFlags flags
	){

	return VkFenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
	};
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(
		VkSemaphoreCreateFlags flags
	){

	return VkSemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
	};
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(
		VkCommandBufferUsageFlags flags,
		VkCommandBufferInheritanceInfo* inheritance_info
	){

	return VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = flags,
		.pInheritanceInfo = inheritance_info,
	};
}

VkPipelineShaderStageCreateInfo vkinit::shader_stage_create_info(
		VkShaderStageFlagBits stage,
		VkShaderModule shader
	){

	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = stage,
		.module = shader,
		.pName = "main",
	};
}


VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info(

	){

	return VkPipelineVertexInputStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_state_create_info(
		VkPrimitiveTopology topology
	){

	return VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = topology,
		.primitiveRestartEnable = false,
	};
}


VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(
		VkPolygonMode polygon_mode
	){

	return VkPipelineRasterizationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = polygon_mode,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f,
	};
}


VkPipelineMultisampleStateCreateInfo vkinit::multisample_state_create_info(

	){

	return VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state(

	){

	return VkPipelineColorBlendAttachmentState{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout(

	){

	return VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};
}
