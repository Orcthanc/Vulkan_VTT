//we will be using glsl version 4.5 syntax
#version 450

layout( location = 0 ) in vec3 vPos;
layout( location = 1 ) in vec3 vNorm;
layout( location = 2 ) in vec3 vCol;
layout( location = 3 ) in vec4 vUV1UV2;

layout( location = 0 ) out vec3 fragCol;
layout( location = 1 ) out vec4 fUV1UV2;

layout( set = 0, binding = 0 ) uniform CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 view_proj;
} cam_data;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 model;
} PushConstants;

void main()
{
	gl_Position = cam_data.view_proj * PushConstants.model * vec4( vPos, 1.0f );
	fragCol = vCol;
	fUV1UV2 = vUV1UV2;
}
