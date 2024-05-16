#ifndef VKRNDR_VULKAN_MEMORY_INCLUDED
#define VKRNDR_VULKAN_MEMORY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    [[nodiscard]] uint32_t find_memory_type(VkPhysicalDevice physical_device,
        uint32_t type_filter,
        VkMemoryPropertyFlags properties);

    struct [[nodiscard]] mapped_memory final
    {
        VkDeviceMemory device_memory;
        void* mapped_memory;

        template<typename T>
        [[nodiscard]] T* as(size_t offset = 0)
        {
            // NOLINTBEGIN
            auto* const byte_view{reinterpret_cast<std::byte*>(mapped_memory)};
            return reinterpret_cast<T*>(byte_view + offset);
            // NOLINTEND
        }

        template<typename T>
        [[nodiscard]] T const* as(size_t offset = 0) const
        {
            // NOLINTBEGIN
            auto const* const byte_view{
                reinterpret_cast<std::byte const*>(mapped_memory)};
            return reinterpret_cast<T const*>(byte_view + offset);
            // NOLINTEND
        }
    };

    mapped_memory map_memory(vulkan_device* device,
        VkDeviceMemory memory,
        VkDeviceSize size,
        VkDeviceSize offset = 0);

    void unmap_memory(vulkan_device* device, mapped_memory* memory);
} // namespace vkrndr

#endif // !VKRNDR_VULKAN_MEMORY_INCLUDED
