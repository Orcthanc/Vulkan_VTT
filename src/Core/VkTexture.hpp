#pragma once

#include "Core/VkTypes.hpp"

struct VkEngine;

namespace vkutil {
	bool load_image_file( VkEngine& engine, const char* path, AllocatedImage& img );
}
