#ifndef VKRNDR_VULKAN_BUFFER_INCLUDED
#define VKRNDR_VULKAN_BUFFER_INCLUDED

#include <vulkan/vulkan_core.h>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    struct [[nodiscard]] vulkan_buffer final
    {
        VkBuffer buffer{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkDeviceSize size{};
    };

    void destroy(vulkan_device const* device, vulkan_buffer* buffer);

    vulkan_buffer create_buffer(vulkan_device const* device,
        VkDeviceSize size,
        VkBufferCreateFlags usage,
        VkMemoryPropertyFlags memory_properties);
} // namespace vkrndr
#endif
