find_package( Vulkan REQUIRED )
find_package( glm REQUIRED )
find_package( SDL2 REQUIRED )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

add_executable( ${PROJECT_NAME}
	Core/VkEngine.cpp
	Core/VkInit.cpp
	Core/VkMesh.cpp
	Core/VkTexture.cpp
	Core/main.cpp )

if( NO_FILE_PREFIX )
	target_compile_definitions( ${PROJECT_NAME} PUBLIC NO_FILE_PREFIX )
endif( NO_FILE_PREFIX )

target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( ${PROJECT_NAME} vkbootstrap Vulkan::Vulkan SDL2::SDL2 vma stb )

if(WIN32)
	target_link_libraries( ${PROJECT_NAME} glm::glm )
else(WIN32)
	target_link_libraries( ${PROJECT_NAME} glm )
endif(WIN32)


add_dependencies( ${PROJECT_NAME} Shaders )
