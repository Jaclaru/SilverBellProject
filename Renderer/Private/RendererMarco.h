#pragma once

// Windows 平台下动态库导出/导入设置
#ifdef _WIN32
    #ifdef RENDERER_EXPORTS

        #define RENDERER_API __declspec(dllexport)

    #else

        #define RENDERER_API __declspec(dllimport)

    #endif

#else

    #define RENDERER_API

#endif

#define VULKAN_DEBUG_ENABLE 0