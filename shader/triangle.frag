//glsl version 4.5
#version 450

layout( set = 1, binding = 0 ) uniform sampler2D tex1;

layout( location = 0 ) in vec3 fragCol;
layout( location = 1 ) in vec4 UV1UV2;

layout (location = 0) out vec4 outFragColor;

void main()
{
	//outFragColor = vec4( fragCol, 1.0f );
	//outFragColor = vec4( UV1UV2.xy, 0.0f, 1.0f );
	outFragColor = vec4( texture( tex1, UV1UV2.xy ).xyz, 1.0f );
}

