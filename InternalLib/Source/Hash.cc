#include "Hash.hh"

#include <xxHash/xxh3.h>

using namespace SilverBell::Algorithm;

std::uint64_t HashFunction::Hash64(const void* Data, const std::uint32_t Size)
{
    return XXH3_64bits(Data, Size);
}
