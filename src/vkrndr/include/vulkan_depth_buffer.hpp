#ifndef VKRNDR_VULKAN_DEPTH_BUFFER_INCLUDED
#define VKRNDR_VULKAN_DEPTH_BUFFER_INCLUDED

#include <vulkan_image.hpp>

#include <vulkan/vulkan_core.h>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    vulkan_image create_depth_buffer(vulkan_device* device, VkExtent2D extent);
} // namespace vkrndr

#endif
