#ifndef VKRNDR_VULKAN_UTILITY_INCLUDED
#define VKRNDR_VULKAN_UTILITY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <source_location>
#include <span>
#include <utility>
#include <vector>

namespace vkrndr
{
    VkResult check_result(VkResult result,
        std::source_location source_location = std::source_location::current());

    template<typename T>
    [[nodiscard]] constexpr uint32_t count_cast(T const count)
    {
        assert(std::in_range<uint32_t>(count));
        return static_cast<uint32_t>(count);
    }

    template<typename T>
    [[nodiscard]] std::span<std::byte const> as_bytes(T const& value,
        size_t const size = sizeof(T))
    {
        // NOLINTNEXTLINE
        return {reinterpret_cast<std::byte const*>(&value), size};
    }

    template<typename T>
    [[nodiscard]] std::span<std::byte const> as_bytes(
        std::vector<T> const& value,
        std::optional<size_t> const elements = std::nullopt)
    {
        // NOLINTNEXTLINE
        return {reinterpret_cast<std::byte const*>(value.data()),
            elements.value_or(value.size()) * sizeof(T)};
    }

} // namespace vkrndr

#endif // !VKRNDR_VULKAN_UTILITY_INCLUDED
