#pragma once

#include <cstdint>

#include "InternalLibMarco.hh"

#include <ylt/reflection/member_value.hpp>

namespace SilverBell::Algorithm
{
    class INTERNALLIB_API HashFunction
    {
    public:
        HashFunction() = delete;

        static std::uint64_t Hash64(const void* Data, std::uint32_t Size);

        template<typename T>
        static std::size_t HashCombine(const T& Data)
        {
            std::size_t Seed = 0;
            ylt::reflection::for_each(Data, [&Seed](auto& Field, auto Name, auto Index)
                {
                    using MemberType = std::remove_cvref_t<decltype(Field)>;
                    size_t ValueHash = 0;
                    if constexpr (requires { typename MemberType::value_type; Field.size(); Field.data(); })
                    {
                        // 是容器类型
                        ValueHash = HashFunction::Hash64(Field.data(), sizeof(MemberType::value_type) * Field.size());
                    }
                    else
                    {
                        ValueHash = HashFunction::Hash64(&Field, sizeof(MemberType));
                    }
                    Seed ^= ValueHash + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
                });
            return Seed;
        }
    };
}
