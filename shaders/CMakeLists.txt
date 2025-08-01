# Check for glslc executable
if(NOT DEFINED Vulkan_GLSLC_EXECUTABLE)
    find_program(Vulkan_GLSLC_EXECUTABLE glslc
        HINTS "$ENV{VULKAN_SDK}/Bin"
              "$ENV{VULKAN_SDK}/bin"
        REQUIRED
    )
endif()

# Shader input/output setup
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(SHADER_SOURCES
    triangle.vert
    triangle.frag
)

set(COMPILED_SHADERS "")

foreach(SHADER IN LISTS SHADER_SOURCES)
    set(INPUT ${SHADER_SOURCE_DIR}/${SHADER})
    set(OUTPUT ${SHADER_BINARY_DIR}/${SHADER}.spv)

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${INPUT} -o ${OUTPUT}
        DEPENDS ${INPUT}
        COMMENT "Compiling ${SHADER}"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})
