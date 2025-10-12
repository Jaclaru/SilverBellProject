#pragma once

#include <concepts>

// 判断一个类型是否是标准容器
template <typename T>
concept IsStdContainer =
    requires(T t) {
    typename T::value_type;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
    { t.data() } -> std::convertible_to<typename T::value_type*>;
};