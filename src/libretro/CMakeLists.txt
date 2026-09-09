# SPDX-License-Identifier: BSD-3-Clause
add_library(sm2_libretro SHARED core.cpp input.cpp)
target_include_directories(sm2_libretro PRIVATE include)
target_link_libraries(sm2_libretro PRIVATE sm2_hw sm2_render sm2_warnings)
set_target_properties(sm2_libretro PROPERTIES PREFIX "" OUTPUT_NAME "sm2_libretro"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libretro"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libretro")
# Only the standard ABI is exported; imported libraries retain their own flags.
if(APPLE)
    target_link_options(sm2_libretro PRIVATE
        "LINKER:-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.macos")
elseif(MINGW)
    # Ship one DLL, without compiler or winpthread runtime DLL dependencies.
    target_link_options(sm2_libretro PRIVATE -static)
elseif(UNIX)
    target_link_options(sm2_libretro PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
endif()
configure_file("${CMAKE_SOURCE_DIR}/data/games.xml"
    "${CMAKE_BINARY_DIR}/libretro/system/sm2-emu/games.xml" COPYONLY)

option(SM2_LIBRETRO_CHECKS "Build ROM-free Libretro input checks" OFF)
if(SM2_LIBRETRO_CHECKS)
    add_executable(sm2-libretro-input-checks checks/input.cpp input.cpp)
    target_include_directories(sm2-libretro-input-checks PRIVATE
        . include "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(sm2-libretro-input-checks PRIVATE sm2_warnings)
endif()

if(SM2_LIBRETRO_VULKAN)
    # Recompile the shared passes with frontend-resolved entry points. No SDL,
    # window/swapchain backend, ImGui, or second Vulkan loader is linked.
    add_library(sm2_libretro_gpu STATIC
        vulkan_renderer.cpp vulkan_negotiation.cpp
        ../render/vk/poly3d_pass.cpp ../render/vk/tilemap_pass.cpp
        ../render/vk/vk_common.cpp ../render/vk/vma_impl.cpp)
    target_include_directories(sm2_libretro_gpu PRIVATE . include "${SM2_VULKAN_INCLUDE_DIR}")
    target_compile_definitions(sm2_libretro_gpu PRIVATE VK_NO_PROTOTYPES)
    if(MSVC)
        target_compile_options(sm2_libretro_gpu PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/vk_dispatch.h")
    else()
        target_compile_options(sm2_libretro_gpu PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/vk_dispatch.h")
    endif()
    target_link_libraries(sm2_libretro_gpu PRIVATE sm2_hw sm2_render sm2_shaders
        sm2_warnings GPUOpen::VulkanMemoryAllocator)
    target_compile_definitions(sm2_libretro PRIVATE SM2_LIBRETRO_VULKAN=1)
    target_link_libraries(sm2_libretro PRIVATE sm2_libretro_gpu)
endif()

option(SM2_LIBRETRO_GPU_CHECKS "Build the separate Vulkan test frontend" ${SM2_LIBRETRO_CHECKS})
if(SM2_LIBRETRO_VULKAN AND SM2_LIBRETRO_GPU_CHECKS)
    # A real GPU test frontend, never linked into the core.
    find_library(SM2_TEST_VULKAN_LOADER NAMES vulkan vulkan-1 REQUIRED)
    add_library(sm2-vulkan-test-host SHARED checks/vulkan_host.cpp)
    target_include_directories(sm2-vulkan-test-host PRIVATE include "${SM2_VULKAN_INCLUDE_DIR}")
    target_link_libraries(sm2-vulkan-test-host PRIVATE "${SM2_TEST_VULKAN_LOADER}")
    set_target_properties(sm2-vulkan-test-host PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libretro-checks")
endif()
