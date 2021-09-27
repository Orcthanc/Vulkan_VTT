#pragma once

#include "VkTypes.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <vulkan/vulkan_core.h>

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec4 uv1_uv2;

	static VertexInputDescription get_vk_description();
};

struct PushConstants {
	glm::vec4 data;
	glm::mat4 camera;
};

struct Mesh {
	std::vector<Vertex> vertices;
	AllocatedBuffer buffer;
};
