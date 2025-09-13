#pragma once

// Windows 平台下动态库导出/导入设置
#ifdef _WIN32
#ifdef INTERNALLIB_EXPORTS

#define INTERNALLIB_API __declspec(dllexport)

#else

#define INTERNALLIB_API __declspec(dllimport)

#endif

#else

#define INTERNALLIB_API

#endif