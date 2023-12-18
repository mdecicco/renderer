cmake_minimum_required(VERSION 3.15)
project(render)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib/debug)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib/release)

set (all_sources "")
macro(add_sources group dir)
    set(_all "")

    file(GLOB _inc "include/render/${dir}/*.h" "include/render/${dir}/*.hpp" "include/render/${dir}/*.inl")
    list(APPEND _all ${_inc})

    file(GLOB _src "src/${dir}/*.cpp")
    list(APPEND _all ${_src})

    list(APPEND all_sources ${_all})
    string(REPLACE "/" "\\" group_path "${group}")
    source_group(${group_path} FILES ${_all})

    unset(_inc)
    unset(_src)
    unset(_all)
endmacro(add_sources)

add_sources("src" ".")
add_sources("src" "vulkan")
add_sources("src" "core")
add_sources("src" "utils")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory("./deps")
add_subdirectory("./test")

add_library(render ${all_sources})

find_package(Vulkan REQUIRED)

target_include_directories(render PUBLIC src)
target_include_directories(render PUBLIC
    include
    ${Vulkan_INCLUDE_DIRS}
    ./deps/utils/include
    ./deps/glslang
    ./deps/imgui
)

SET(debug_libs
    Vulkan::Vulkan
    glslang
    SPIRV
    glslang-default-resource-limits
    ImGui
)

SET(release_libs
    Vulkan::Vulkan
    glslang
    SPIRV
    glslang-default-resource-limits
    ImGui
)

target_link_libraries(render ${debug_libs})
#target_link_libraries(render ${release_libs})

set_property(TARGET render PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")

add_definitions(-D_CRT_NO_VA_START_VALIDATION)