#include <vulkan_buffer.hpp>

#include <vulkan_device.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_utility.hpp>

void vkrndr::destroy(vulkan_device const* device, vulkan_buffer* const buffer)
{
    if (buffer)
    {
        vkDestroyBuffer(device->logical, buffer->buffer, nullptr);
        vkFreeMemory(device->logical, buffer->memory, nullptr);
    }
}

vkrndr::vulkan_buffer vkrndr::create_buffer(vulkan_device const* const device,
    VkDeviceSize const size,
    VkBufferCreateFlags const usage,
    VkMemoryPropertyFlags const memory_properties)
{
    vulkan_buffer rv{};
    rv.size = size;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check_result(
        vkCreateBuffer(device->logical, &buffer_info, nullptr, &rv.buffer));

    VkMemoryRequirements memory_requirements; // NOLINT
    vkGetBufferMemoryRequirements(device->logical,
        rv.buffer,
        &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = vkrndr::find_memory_type(device->physical,
        memory_requirements.memoryTypeBits,
        memory_properties);

    check_result(
        vkAllocateMemory(device->logical, &alloc_info, nullptr, &rv.memory));

    check_result(vkBindBufferMemory(device->logical, rv.buffer, rv.memory, 0));

    return rv;
}
