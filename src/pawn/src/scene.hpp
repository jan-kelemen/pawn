#ifndef PAWN_SCENE_INLCUDED
#define PAWN_SCENE_INLCUDED

#include <vulkan_buffer.hpp>
#include <vulkan_scene.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>

namespace vkrndr
{
    struct vulkan_device;
    struct vulkan_pipeline;
    class vulkan_renderer;
    class vulkan_swap_chain;
} // namespace vkrndr

namespace pawn
{
    class [[nodiscard]] scene final : public vkrndr::vulkan_scene
    {
    public: // Construction
        scene();

        scene(scene const&) = delete;

        scene(scene&&) noexcept = delete;

    public: // Destruction
        ~scene() override;

    public: // Interface
        void attach_renderer(vkrndr::vulkan_device* device,
            vkrndr::vulkan_swap_chain const* swap_chain,
            vkrndr::vulkan_renderer* renderer);

        void detach_renderer();

    public: // vulkan_scene overrides
        VkClearValue clear_color() override;

        void draw(VkCommandBuffer command_buffer, VkExtent2D extent) override;

        void draw_imgui() override;

    public: // Operators
        scene& operator=(scene const&) = delete;

        scene& operator=(scene&&) noexcept = delete;

    private: // Data
        vkrndr::vulkan_device* vulkan_device_{};
        vkrndr::vulkan_renderer* vulkan_renderer_{};

        std::unique_ptr<vkrndr::vulkan_pipeline> pipeline_;

        vkrndr::vulkan_buffer vert_index_buffer_;
    };
} // namespace pawn

#endif
