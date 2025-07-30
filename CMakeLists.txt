cmake_minimum_required(VERSION 3.10)

project(ArtifactVK)

set(SOURCE)
set(HEADERS)

add_subdirectory(src)

message(STATUS "Collected source files:")
foreach(src ${SOURCE})
    message(STATUS "  ${src}")
endforeach()


add_executable(ArtifactVK ${SOURCE} ${HEADERS})
target_include_directories(ArtifactVK PRIVATE include)
target_include_directories(ArtifactVK PUBLIC external)