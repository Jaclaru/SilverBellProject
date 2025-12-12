#pragma once

#include "FunctionTraits.hh"

#include "Logger.hh"

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
                        catch (const std::exception& E)
                        {
                            LOG_WARN("回调函数异常：{}", E.what());
                            throw std::runtime_error("回调函数异常：" + std::string(E.what()));
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
                catch (const std::exception& E)
                {
                    LOG_ERROR("回调函数出错：{}", E.what());
                }
            }
        }

        void ClearCallbacks() { Callbacks.clear(); }

        void ClearParameters() { Params.clear(); }

        [[maybe_unused]] size_t CallbackCount() const { return Callbacks.size(); }   // NOLINT(modernize-use-nodiscard)

        [[maybe_unused]] size_t ParameterCount() const { return Params.size(); }  // NOLINT(modernize-use-nodiscard)

        // 获取指定索引的参数
        template<typename T>
        requires IsInVariantValue<T, VariantType>
        [[nodiscard]] T GetParameter(size_t Index) const
        {
            if (Index >= Params.size()) 
            {
                LOG_ERROR("参数索引超出范围！");
                throw std::out_of_range("Parameter index out of range");
            }
            try 
            {
                return std::get<T>(Params[Index]);
            }
            catch (const std::bad_variant_access&) 
            {
                LOG_ERROR("参数类型不匹配！");
                throw std::runtime_error("Parameter type mismatch");
            }
        }

        // 检查指定索引的参数是否为特定类型
        template<typename T>
        requires IsInVariantValue<T, VariantType>
        [[nodiscard]] bool IsParameterType(size_t Index) const
        {
            if (Index >= Params.size()) 
            {
                LOG_DEBUG("参数索引超出范围！");
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
        Traits::ArgsTuple ExtractArgs()
        {
            return [this]<size_t... I>(std::index_sequence<I...>)
            {
                return typename Traits::ArgsTuple{ExtractArg<typename Traits::template ArgType<I>>()...};
            }(std::make_index_sequence<Traits::Arity>{});
        }

        template<typename T>
        requires IsInVariantValue<std::remove_cvref_t<T>, VariantType>
        T ExtractArg()
        {
            using TNoRef = std::remove_cvref_t<T>;
            for (auto& Param : Params)
            {
                if (std::holds_alternative<TNoRef>(Param))
                {
                    return std::get<TNoRef>(Param);
                }
            }
            LOG_ERROR("参数类型不匹配，提取失败！");
            throw std::runtime_error("Parameter type mismatch!");
        }

        std::vector<VariantType> Params;
        std::vector<std::function<void()>> Callbacks;
    };

}
