#pragma once

#include "FunctionTraits.hh"

#include <spdlog/spdlog.h>

#include <vector>
#include <variant>

namespace SilverBell::TMP
{
    // 检查类型是否在 variant 中
    template<typename T, typename Variant>
    struct IsInVariant;

    template<typename T, typename... Types>
    struct IsInVariant<T, std::variant<Types...>>
        : std::disjunction<std::is_same<T, Types>...> {};

    template<typename T, typename Variant>
    constexpr bool IsInVariantValue = IsInVariant<T, Variant>::value;

    // 检查类型是否可以转换为 variant 中的某个类型
    template<typename T, typename Variant>
    struct IsConvertibleToVariant;

    template<typename T, typename... Types>
    struct IsConvertibleToVariant<T, std::variant<Types...>>
        : std::disjunction<std::is_convertible<T, Types>...> {};

    template<typename T, typename Variant>
    constexpr bool IsConvertibleToVariantValue = IsConvertibleToVariant<T, Variant>::value;

    template<typename VariantType = std::variant<int, double, std::string, const char*>>
    class GenericCallbackSystem
    {
    public:

        template<typename T>
        requires IsConvertibleToVariantValue<T, VariantType>
        void SetParameter(size_t Index, T Value)
        {
            if (Index >= Params.size())
            {
                Params.resize(Index + 1);
            }

            if constexpr (IsInVariantValue<T, VariantType>)
            {
                Params[Index] = std::move(Value);
            }
            else
            {
                Params[Index] = ConvertToVariant<T>(std::move(Value));
            }
        }

        // 注册回调函数
        template<typename Func>
        void RegisterCallback(Func&& Callback)
        {
            using Traits = FunctionTraits<std::decay_t<Func>>;

            if (Traits::Arity > Params.size())
            {
                return;
            }
            if (!CheckParameterTypes<Traits>())
            {
                return;
            }
            auto Invoker = [this, F = std::forward<Func>(Callback)]() -> void
            {
                if constexpr (Traits::Arity == 0)
                {
                    F();
                }
                else
                {
                    try
                    {
                        auto Args = ExtractArgs<Traits>();
                        std::apply(F, Args);
                    }
                    catch (const std::exception& e)
                    {
                        // 处理类型转换错误
                        // 可以选择记录日志或抛出异常
                        spdlog::warn("回调需要的参数超出！注册失败！");
                        throw std::runtime_error("参数类型不匹配: " + std::string(e.what()));
                    }
                }
            };
            Callbacks.push_back(Invoker);
        }

        template<typename Func>
        void Connect(Func&& F)
        {
            RegisterCallback(std::forward<Func>(F));
        }

        // 触发所有回调
        void Trigger() const
        {
            for (auto& Callback : Callbacks) 
            {
                try 
                {
                    Callback();
                }
                catch (const std::exception& e) 
                {
                    spdlog::error("回调函数出错：{}", e.what());
                }
            }
        }

        void ClearCallbacks() { Callbacks.clear(); }

        void ClearParameters() { Params.clear(); }

        size_t CallbackCount() const { return Callbacks.size(); }

        size_t ParameterCount() const { return Params.size(); }

        // 获取指定索引的参数
        template<typename T>
        requires IsInVariantValue<T, VariantType>
        [[nodiscard]] T GetParameter(size_t Index) const
        {
            if (Index >= Params.size()) 
            {
                spdlog::error("参数索引超出范围！");
                throw std::out_of_range("Parameter index out of range");
            }
            try 
            {
                return std::get<T>(Params[Index]);
            }
            catch (const std::bad_variant_access&) 
            {
                throw std::runtime_error("Parameter type mismatch");
            }
        }

        // 检查指定索引的参数是否为特定类型
        template<typename T>
        requires IsInVariantValue<T, VariantType>
        bool IsParameterType(size_t Index) const
        {
            if (Index >= Params.size()) 
            {
                return false;
            }
            return std::holds_alternative<T>(Params[Index]);
        }

    private:
        // 将类型转换为 variant 中的类型
        template<typename T>
        VariantType ConvertToVariant(T Value)
        {
            // 这里可以支持更多类型的转换
            if constexpr (std::is_convertible_v<T, std::string>) 
            {
                return std::string(std::move(Value));
            }
            else 
            {
                // 对于其他类型，尝试直接转换
                return VariantType(std::move(Value));
            }
        }

        // 检查函数参数类型是否与存储的参数类型兼容
        template<typename Traits>
        bool CheckParameterTypes()
        {
            return [this]<size_t... I>(std::index_sequence<I...>)
            {
                return ((IsConvertibleToVariantValue<typename Traits::template ArgType<I>, VariantType>) && ...);
            }(std::make_index_sequence<Traits::Arity>{});
        }

        // 提取参数并转换为函数需要的类型
        template<typename Traits>
        typename Traits::ArgsTuple ExtractArgs()
        {
            return [this]<size_t... I>(std::index_sequence<I...>)
            {
                return typename Traits::ArgsTuple{ExtractArg<typename Traits::template ArgType<I>>(I)...};
            }(std::make_index_sequence<Traits::Arity>{});
        }

        // 提取单个参数并转换为指定类型
        template<typename T>
        T ExtractArg(size_t Index)
        {
            if (Index >= Params.size())
            {
                spdlog::error("提取的索引超出范围！");
                throw std::runtime_error("Parameter not found at index " + std::to_string(Index));
            }
            try 
            {

                return std::visit([](auto&& Arg) -> T 
                {
                    using U = std::decay_t<decltype(Arg)>;
                    if constexpr (std::is_convertible_v<U, T>) 
                    {
                        return static_cast<T>(Arg);
                    }
                    else 
                    {
                        throw std::bad_variant_access();
                    }
                }, Params[Index]);

            }
            catch (const std::bad_variant_access&)
            {
                spdlog::error("参数类型不匹配，提取失败！");
                throw std::runtime_error("Parameter type mismatch at index " + std::to_string(Index));
            }

        }

        std::vector<VariantType> Params;
        std::vector<std::function<void()>> Callbacks;
    };

}
