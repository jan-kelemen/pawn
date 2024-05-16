#include <vulkan_memory.hpp>

#include <vulkan_device.hpp>
#include <vulkan_utility.hpp>

#include <stdexcept>

uint32_t vkrndr::find_memory_type(VkPhysicalDevice const physical_device,
    uint32_t const type_filter,
    VkMemoryPropertyFlags const properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        bool const is_type_set{(type_filter & (1 << i)) != 0};
        if (is_type_set &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }

    throw std::runtime_error{"failed to find suitable memory type!"};
}

vkrndr::mapped_memory vkrndr::map_memory(vulkan_device* const device,
    VkDeviceMemory const memory,
    VkDeviceSize const size,
    VkDeviceSize const offset)
{
    void* mapped; // NOLINT
    check_result(
        vkMapMemory(device->logical, memory, offset, size, 0, &mapped));
    return {memory, mapped};
}

void vkrndr::unmap_memory(vulkan_device* device, mapped_memory* memory)
{
    vkUnmapMemory(device->logical, memory->device_memory);
}
