#ifndef CPPEXT_NUMERIC_INCLUDED
#define CPPEXT_NUMERIC_INCLUDED

#include <cassert>
#include <concepts>
#include <utility>

namespace cppext
{
    template<std::integral To>
    [[nodiscard]] constexpr To narrow(std::integral auto value)
    {
        assert(std::in_range<To>(value));
        return static_cast<To>(value);
    }
} // namespace cppext

#endif
