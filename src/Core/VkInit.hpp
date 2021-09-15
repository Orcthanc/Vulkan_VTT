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
}
