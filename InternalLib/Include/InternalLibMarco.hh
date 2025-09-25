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

/**
 * Implement logical operators on a class enum for making it usable as a flags enum.
 */
 // clang-format off
#define FALCOR_ENUM_CLASS_OPERATORS(e_) \
    __FORCEINLINE e_ operator& (e_ a, e_ b) { return static_cast<e_>(static_cast<int>(a)& static_cast<int>(b)); } \
    __FORCEINLINE e_ operator| (e_ a, e_ b) { return static_cast<e_>(static_cast<int>(a)| static_cast<int>(b)); } \
    __FORCEINLINE e_& operator|= (e_& a, e_ b) { a = a | b; return a; }; \
    __FORCEINLINE e_& operator&= (e_& a, e_ b) { a = a & b; return a; }; \
    __FORCEINLINE e_  operator~ (e_ a) { return static_cast<e_>(~static_cast<int>(a)); } \
    __FORCEINLINE bool is_set(e_ val, e_ flag) { return (val & flag) != static_cast<e_>(0); } \
    __FORCEINLINE void flip_bit(e_& val, e_ flag) { val = is_set(val, flag) ? (val & (~flag)) : (val | flag); }