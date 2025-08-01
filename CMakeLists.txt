cmake_minimum_required(VERSION 3.8)

project(ArtifactVK)

find_package(Vulkan REQUIRED FATAL_ERROR)

set(SOURCE)
set(HEADERS)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)

add_subdirectory(src)
add_subdirectory(shaders)

add_library(glfw STATIC IMPORTED)
# TODO: Select correct runtime for glfw3 based on selection above
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/glfw-3.4.bin.WIN64/include"
)

add_executable(ArtifactVK ${SOURCE} ${HEADERS})
target_include_directories(ArtifactVK 
							PRIVATE include
							PUBLIC external/glfw/glfw-3.4.bin.WIN64/include
							PUBLIC external)
target_link_libraries(ArtifactVK 
						PRIVATE Vulkan::Vulkan 
						PRIVATE glfw)
set_property(TARGET ArtifactVK PROPERTY CXX_STANDARD 20)
set_property(TARGET ArtifactVK PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")


if (MSVC)
    target_compile_options(ArtifactVK PRIVATE /MP)
	target_compile_definitions(ArtifactVK PRIVATE 
		$<$<CONFIG:Debug>:_DEBUG;_ITERATOR_DEBUG_LEVEL=2;_CRT_SECURE_NO_WARNINGS>
	)

	target_compile_options(ArtifactVK PRIVATE 
		$<$<CONFIG:Debug>:/RTC1>
	)
endif()

set(SRC "${CMAKE_SOURCE_DIR}/textures")
set(DST "${CMAKE_BINARY_DIR}")

add_custom_target(link_or_copy_textures ALL
    COMMAND ${CMAKE_COMMAND} 
        -D SRC=${SRC}
        -D DST=${DST}
		-P ${CMAKE_SOURCE_DIR}/cmake/link_assets.cmake
    COMMENT "Linking or copying textures directory"
)

add_dependencies(ArtifactVK Shaders link_or_copy_textures)
