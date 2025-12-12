cmake_minimum_required (VERSION 3.19 FATAL_ERROR)

project(SilverBellEngine)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置策略CMP0091为NEW，新策略
if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif (POLICY CMP0091)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 判断操作系统类型
if(WIN32)
    # 如果是 Windows
    message("当前操作系统为 Windows")
    # 添加预定义宏
    add_definitions(
        -D__FORCEINLINE=__forceinline
        -DNOMINMAX
        -DPLATFORM_WINDOWS=1
    )

    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_definitions(UNICODE _UNICODE)

    #设置生成平台标记
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OPT_HINT Win64)
    else()
        set(OPT_HINT Win32)
    endif()

elseif(UNIX AND NOT APPLE)
    # 如果是 Linux
    message("当前操作系统为 Linux")
    add_definitions(-D__ForceInline=inline)
else()
    message(FATAL_ERROR "不支持的操作系统类型")
endif()

# 设置项目根目录宏，方便在代码中使用
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}/")

# 判断当前的构建版本
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_HINT Debug)
else()
    set(BUILD_HINT Release)
endif()

message("当前构建的版本是: ${BUILD_HINT}")
message("当前构建的平台是: ${OPT_HINT}")

########################## 项目配置 ##########################

#set(SILVER_BELL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
#set(CMAKE_INSTALL_PREFIX "${SILVER_BELL_ROOT_DIR}/bin")
#set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)
message("二进制文件夹位置:${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ")

########################## 第三方库配置 ##########################

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/ThirdParty)
set(THIRD_PARTY_LIB_DIR "Library/${OPT_HINT}/${BUILD_HINT}")

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

# spdlog
set(spdlog_DIR "${THIRD_PARTY_DIR}/spdlog/${OPT_HINT}/lib/cmake/spdlog" CACHE INTERNAL "spdlog CMake directory")
message("spdlog_DIR: ${spdlog_DIR}")
find_package(spdlog REQUIRED)

# Eigen
set(EIGEN_INCLUDE_DIR "${THIRD_PARTY_DIR}/Eigen/Include" CACHE INTERNAL "Eigen Include")

# yalantinglibs
include(FetchContent)
FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG 0.5.5
    GIT_SHALLOW 1 # optional ( --depth=1 )
)
FetchContent_MakeAvailable(yalantinglibs)

# stb
set(STB_INCLUDE_DIR "${THIRD_PARTY_DIR}/stb/Include" CACHE INTERNAL "stb Include")

# tinyobjloader
set(TINYOBJLOADER_INCLUDE_DIR "${THIRD_PARTY_DIR}/tinyobjloader/Include" CACHE INTERNAL "tinyobjloader Include")

# xxHash
set(XXHASH_INCLUDE_DIR "${THIRD_PARTY_DIR}/xxHash/Include" CACHE INTERNAL "xxHash Include")

# SPIRV-Reflect 要作为子目录编译
FetchContent_Declare(
    SPIRV-Reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG vulkan-sdk-1.4.313.0 
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(SPIRV-Reflect)

########################## 子目录 ##########################
add_subdirectory(Application)
add_subdirectory(Renderer)
add_subdirectory(InternalLib)
