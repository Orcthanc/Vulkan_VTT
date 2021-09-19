cmake_minimum_required( VERSION 3.8 )

project( "VTT" )

option( NO_FILE_PREFIX "Assumes the shader folder is copied to the executable folder" OFF )

add_subdirectory( external )

add_subdirectory( src )

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shader/*.frag"
    "${PROJECT_SOURCE_DIR}/shader/*.vert"
    "${PROJECT_SOURCE_DIR}/shader/*.comp"
    )

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${PROJECT_SOURCE_DIR}/shader/${FILE_NAME}.spv")
	##execute glslang command to compile that specific shader
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
		COMMENT "Compiling shader ${GLSL}"
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
