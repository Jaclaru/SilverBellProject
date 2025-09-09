#pragma once

#include <functional>

namespace SilverBell::TMP
{
    // 函数特征提取 Traits
    template<typename T>
    struct FunctionTraits;

    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(Args...)>
    {
        static constexpr size_t Arity = sizeof...(Args);
        using ResultType = Ret;
        using ArgsTuple = std::tuple<Args...>;
        template<size_t I>
        using ArgType = std::tuple_element_t<I, ArgsTuple>;
    };

    // 函数指针
    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret(*)(Args...)> : FunctionTraits<Ret(Args...)> {};

    // 对于std::function的特化
    template<typename Ret, typename... Args>
    struct FunctionTraits<std::function<Ret(Args...)>> : FunctionTraits<Ret(Args...)> {};

    // 函数对象（仿函数、lambda等）
    template<typename Callable>
    struct FunctionTraits : FunctionTraits< decltype(&Callable::operator())> {};

    // 对于成员函数的特化
#define FUNCTION_TRAITS(...)\
template <typename Ret, typename Class, typename... Args>\
struct FunctionTraits<Ret(Class::*)(Args...) __VA_ARGS__> : FunctionTraits<Ret(Args...)> {};\

    FUNCTION_TRAITS()
    FUNCTION_TRAITS(const) // Lambda 表达式默认生成 const 成员函数的 operator()，如果没有定义const成员函数匹配将会引发错误
    FUNCTION_TRAITS(volatile)
    FUNCTION_TRAITS(const volatile)

    // 辅助别名模板
    template<typename T>
    using FunctionReturnType = typename FunctionTraits<T>::ResultType;

    template<typename T>
    using FunctionArgsTuple = typename FunctionTraits<T>::ArgsTuple;

    template<typename T>
    constexpr size_t FunctionArityValue = FunctionTraits<T>::Arity;

    template<typename T, size_t I>
    using FunctionArgType = typename FunctionTraits<T>::template ArgType<I>;
}
