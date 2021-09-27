#include "Core/VkTexture.hpp"

#include "Core/VkEngine.hpp"
#include "Core/VkInit.hpp"
#include "Core/VkTypes.hpp"
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

bool vkutil::load_image_file( VkEngine& engine, const char* path, AllocatedImage& image ){
	int width, height, channels;

	stbi_uc* data = stbi_load( path, &width, &height, &channels, STBI_rgb_alpha );

	if( !data ){
		std::cout << "Failed to load texture " << path << std::endl;
		return false;
	}

	VkDeviceSize data_size = width * height * 4;
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	AllocatedBuffer staging = engine.create_buffer( data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY );

	void* gpu_data;
	vmaMapMemory( engine.vma_alloc, staging.allocation, &gpu_data );
	memcpy( gpu_data, data, static_cast<size_t>( data_size ));
	vmaUnmapMemory( engine.vma_alloc, staging.allocation );

	stbi_image_free( data );


	VkExtent3D img_size {
		.width = static_cast<uint32_t>( width ),
		.height = static_cast<uint32_t>( height ),
		.depth = 1,
	};

	auto img_cr_inf = vkinit::image_create_info( format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, img_size );

	AllocatedImage img;
	VmaAllocationCreateInfo img_alloc {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	vmaCreateImage( engine.vma_alloc, &img_cr_inf, &img_alloc, &img.image, &img.allocation, nullptr );

	engine.immediate_submit( [&]( VkCommandBuffer buf ){
			VkImageSubresourceRange range {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};

			VkImageMemoryBarrier to_transfer {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = img.image,
				.subresourceRange = range,
			};

			vkCmdPipelineBarrier(
					buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &to_transfer );

			VkBufferImageCopy img_cpy {
				.bufferOffset = 0,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = VkImageSubresourceLayers{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.imageExtent = img_size,
			};

			vkCmdCopyBufferToImage( buf, staging.buffer, img.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy );

			VkImageMemoryBarrier to_shader {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = img.image,
				.subresourceRange = range,
			};

			vkCmdPipelineBarrier(
					buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &to_shader );
		});

	vmaDestroyBuffer( engine.vma_alloc, staging.buffer, staging.allocation );

	engine.deletion_queue.emplace_function( [&engine, img](){
			vmaDestroyImage( engine.vma_alloc, img.image, img.allocation );
		});

	image = img;

	std::cout << "Loaded image " << path << std::endl;

	return true;
}
