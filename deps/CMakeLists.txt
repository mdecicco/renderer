set(BUILD_TESTING           OFF CACHE BOOL "" FORCE)
set(UTILS_BUILD_PLAYGROUNDS OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST            OFF CACHE BOOL "Enables testing" FORCE)
set(BUILD_EXTERNAL          OFF CACHE BOOL "Build external dependencies in /External" FORCE)
set(ENABLE_SPVREMAPPER      OFF CACHE BOOL "Enables building of SPVRemapper" FORCE)
set(ENABLE_HLSL             OFF CACHE BOOL "Enables HLSL input support" FORCE)
set(ENABLE_OPT              OFF CACHE BOOL "Enables spirv-opt capability if present" FORCE)
set(ENABLE_PCH              OFF CACHE BOOL "Enables Precompiled header" FORCE)

add_subdirectory("./utils")
add_subdirectory("./glslang")

cmake_minimum_required(VERSION 3.15)
project(imgui)

file(GLOB IMGUI_SOURCES ./imgui/*.cpp)
set(IMGUI_SOURCES ${IMGUI_SOURCES} ./imgui/backends/imgui_impl_vulkan.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(ImGui PUBLIC ./imgui ${Vulkan_INCLUDE_DIRS})
target_link_libraries(ImGui Vulkan::Vulkan)