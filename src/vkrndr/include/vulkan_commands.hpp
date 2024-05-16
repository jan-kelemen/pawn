#ifndef VKRNDR_VULKAN_COMMANDS_INCLUDED
#define VKRNDR_VULKAN_COMMANDS_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <span>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    void create_command_buffers(vkrndr::vulkan_device const* device,
        VkCommandPool command_pool,
        uint32_t count,
        VkCommandBufferLevel level,
        std::span<VkCommandBuffer> buffers);

    void begin_single_time_commands(vulkan_device const* device,
        VkCommandPool command_pool,
        uint32_t count,
        std::span<VkCommandBuffer> buffers);

    void end_single_time_commands(vulkan_device const* device,
        VkQueue queue,
        std::span<VkCommandBuffer> command_buffers,
        VkCommandPool command_pool);

    void transition_image(VkImage image,
        VkCommandBuffer command_buffer,
        VkImageLayout old_layout,
        VkImageLayout new_layout);

    void copy_buffer_to_image(VkCommandBuffer command_buffer,
        VkBuffer buffer,
        VkImage image,
        VkExtent2D extent);
} // namespace vkrndr

#endif // !VKRNDR_VULKAN_COMMANDS_INCLUDED
