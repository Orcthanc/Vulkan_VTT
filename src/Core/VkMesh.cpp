#include "VkMesh.hpp"
#include <vulkan/vulkan_core.h>

VertexInputDescription Vertex::get_vk_description(){
	VertexInputDescription desc;

	desc.bindings.emplace_back( VkVertexInputBindingDescription{
			.binding = 0,
			.stride = sizeof( Vertex ),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		});

	desc.attributes.emplace_back( VkVertexInputAttributeDescription{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof( Vertex, pos ),
		});

	desc.attributes.emplace_back( VkVertexInputAttributeDescription{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof( Vertex, normal ),
		});

	desc.attributes.emplace_back( VkVertexInputAttributeDescription{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof( Vertex, color ),
		});

	desc.attributes.emplace_back( VkVertexInputAttributeDescription{
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof( Vertex, uv1_uv2 ),
		});

	return desc;
}
