file(GLOB_RECURSE RendererSource CONFIGURE_DEPENDS *.hh *.cc)

find_package(Vulkan)

add_compile_definitions(RENDERER_EXPORTS)

add_library(Renderer SHARED ${RendererSource})

target_include_directories(Renderer PUBLIC Include)
target_include_directories(Renderer PRIVATE ${Vulkan_INCLUDE_DIRS})

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found. Please ensure Vulkan SDK is installed and the environment variable VULKAN_SDK is set.")
else()

message(STATUS "Vulkan_FOUND: ${Vulkan_FOUND}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan_VERSION: ${Vulkan_VERSION}")
message(STATUS "Vulkan_LIBRARY: ${Vulkan_LIBRARY}")
message(STATUS "Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")

target_link_libraries(Renderer PUBLIC InternalLib)
target_link_libraries(Renderer PRIVATE ${Vulkan_LIBRARIES})

# Vulkan SDK 环境变量
set(VULKAN_SDK $ENV{VULKAN_SDK})

# dxc 头文件和库路径
set(DXC_LIB_DIR "${VULKAN_SDK}/Lib")
set(DXC_LIB "${DXC_LIB_DIR}/dxcompiler.lib")

# 链接 dxcompiler.lib 和 spirv-reflect-static 库
target_link_libraries(Renderer PRIVATE ${DXC_LIB} spirv-reflect-static)

endif()