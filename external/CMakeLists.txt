find_package( Vulkan REQUIRED )

add_library( vkbootstrap STATIC )
add_library( vma INTERFACE )
add_library( stb INTERFACE )

#Vk-Bootstrap
target_sources( vkbootstrap PRIVATE vk_bootstrap/VkBootstrap.cpp )
target_include_directories( vkbootstrap PUBLIC vk_bootstrap )
target_link_libraries( vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}> )

#VMA
target_include_directories( vma INTERFACE vma )

#stb
target_include_directories( stb INTERFACE stb )
