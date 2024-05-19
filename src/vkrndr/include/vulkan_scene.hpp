#ifndef VKRNDR_VULKAN_SCENE_INCLUDED
#define VKRNDR_VULKAN_SCENE_INCLUDED

#include <vulkan/vulkan_core.h>

namespace vkrndr
{
    struct vulkan_image;
} // namespace vkrndr

namespace vkrndr
{
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
    class [[nodiscard]] vulkan_scene
    {
    public: // Destruction
        virtual ~vulkan_scene() = default;

    public: // Interface
        [[nodiscard]] virtual VkClearValue clear_color() = 0;

        [[nodiscard]] virtual VkClearValue clear_depth() = 0;

        [[nodiscard]] virtual vulkan_image* depth_image() = 0;

        virtual void resize(VkExtent2D extent) = 0;

        virtual void draw(VkCommandBuffer command_buffer,
            VkExtent2D extent) = 0;

        virtual void draw_imgui() = 0;
    };
} // namespace vkrndr

#endif
