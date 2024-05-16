#ifndef VKRNDR_VULKAN_IMAGE_INCLUDED
#define VKRNDR_VULKAN_IMAGE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    struct [[nodiscard]] vulkan_image final
    {
        VkImage image{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
    };

    void destroy(vulkan_device const* device, vulkan_image* image);

    vulkan_image create_image(vulkan_device const* device,
        VkExtent2D extent,
        uint32_t mip_levels,
        VkSampleCountFlagBits samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);

    [[nodiscard]] VkImageView create_image_view(vulkan_device const* device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels);

    vulkan_image create_image_and_view(vulkan_device const* device,
        VkExtent2D extent,
        uint32_t mip_levels,
        VkSampleCountFlagBits samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImageAspectFlags aspect_flags);
} // namespace vkrndr

#endif
