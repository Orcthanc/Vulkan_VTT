find_package( Vulkan REQUIRED )
find_package( glm REQUIRED )
find_package( SDL2 REQUIRED )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

add_executable( ${PROJECT_NAME}
	Core/VkEngine.cpp
	Core/VkInit.cpp
	Core/main.cpp )

target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( ${PROJECT_NAME} vkbootstrap glm Vulkan::Vulkan SDL2 vma )
