#pragma once

#include "InternalLibMarco.hh"

#include "ContainerTraits.hh"

#include <ylt/reflection/member_value.hpp>

template<typename T>
concept HasMembers = requires { ylt::reflection::members_count_v<T> != 0; };

namespace SilverBell::Algorithm
{
    class INTERNALLIB_API HashFunction
    {
    public:
        HashFunction() = delete;

        static std::uint64_t Hash64(const void* Data, std::size_t Size);

        __FORCEINLINE static auto HashCombine(auto L, auto R)
        {
            return L ^ (R + 0x9e3779b9 + (L << 6) + (L >> 2));
        }

        template<typename T>
        static std::uint64_t Hash64(const T& Data)
        {
            if constexpr (IsStdContainer<T>)
            {
                using MemberType = typename T::value_type;
                // 如果MemberType仍然是标准容器，则递归调用Hash64
                if constexpr (IsStdContainer<MemberType>)
                {
                    std::uint64_t Seed = 0;
                    for (const auto& Item : Data)
                    {
                        Seed = HashCombine(Seed, Hash64(Item));
                    }
                    return Seed;
                }
                else
                {
                    return Hash64(Data.data(), Data.size() * sizeof(MemberType));
                }
            }
            else
            {
                return Hash64(&Data, sizeof(T));
            }
        }

        template<typename T>
            requires HasMembers<T>
        static std::uint64_t HashCombine(const T& Data)
        {
            std::uint64_t Seed = 0;
            ylt::reflection::for_each(Data, [&Seed](auto& Field, auto Name, auto Index)
            {
                std::uint64_t ValueHash = Hash64(Field);
                Seed = HashCombine(Seed, ValueHash);
            });
            return Seed;
        }
    };
}
