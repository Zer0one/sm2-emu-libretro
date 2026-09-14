# SPDX-License-Identifier: BSD-3-Clause
add_library(sm2_libretro SHARED core.cpp crosshair.cpp input.cpp initial_nvram.cpp nvram_settings.cpp save_ram.cpp)
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
    add_executable(sm2-libretro-input-checks checks/input.cpp crosshair.cpp input.cpp)
    target_include_directories(sm2-libretro-input-checks PRIVATE
        . include "${PROJECT_SOURCE_DIR}/src")
    target_compile_definitions(sm2-libretro-input-checks PRIVATE
        SM2_GAMES_XML_PATH="${PROJECT_SOURCE_DIR}/data/games.xml")
    target_link_libraries(sm2-libretro-input-checks PRIVATE sm2_rom sm2_warnings)
    add_executable(sm2-libretro-save-ram-checks checks/save_ram.cpp initial_nvram.cpp
        nvram_settings.cpp save_ram.cpp)
    target_include_directories(sm2-libretro-save-ram-checks PRIVATE
        . include "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(sm2-libretro-save-ram-checks PRIVATE sm2_warnings)
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

if(SM2_LIBRETRO_OPENGL)
    # Reuse the upstream GL passes with the context and destination framebuffer
    # supplied by Libretro. SDL headers provide Khronos declarations only; no
    # SDL window, platform backend, ImGui renderer, or GL library is linked.
    add_library(sm2_libretro_gl STATIC
        opengl_renderer.cpp
        ../render/gl/gl_common.cpp
        ../render/gl/gl_poly3d_pass.cpp
        ../render/gl/gl_present_pass.cpp
        ../render/gl/gl_tilemap_pass.cpp)
    target_include_directories(sm2_libretro_gl SYSTEM PRIVATE
        "${PROJECT_SOURCE_DIR}/3rdparty/SDL/include")
    target_include_directories(sm2_libretro_gl PRIVATE . include "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(sm2_libretro_gl PRIVATE sm2_hw sm2_render sm2_shaders
        sm2_warnings)
    target_compile_definitions(sm2_libretro PRIVATE SM2_LIBRETRO_OPENGL=1)
    target_link_libraries(sm2_libretro PRIVATE sm2_libretro_gl)
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

option(SM2_LIBRETRO_GL_CHECKS "Build the separate EGL/GLES test frontend" OFF)
if(SM2_LIBRETRO_OPENGL AND SM2_LIBRETRO_GL_CHECKS)
    find_path(SM2_TEST_EGL_INCLUDE_DIR EGL/egl.h REQUIRED)
    find_library(SM2_TEST_EGL_LIBRARY NAMES EGL REQUIRED)
    find_library(SM2_TEST_GLES_LIBRARY NAMES GLESv2 REQUIRED)
    add_library(sm2-opengl-test-host SHARED checks/opengl_host.cpp)
    target_include_directories(sm2-opengl-test-host PRIVATE include "${SM2_TEST_EGL_INCLUDE_DIR}")
    target_link_libraries(sm2-opengl-test-host PRIVATE
        "${SM2_TEST_EGL_LIBRARY}" "${SM2_TEST_GLES_LIBRARY}")
    set_target_properties(sm2-opengl-test-host PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libretro-checks")
endif()
