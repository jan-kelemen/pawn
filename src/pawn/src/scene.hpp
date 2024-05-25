#ifndef PAWN_SCENE_INLCUDED
#define PAWN_SCENE_INLCUDED

#include <vulkan_buffer.hpp>
#include <vulkan_image.hpp>
#include <vulkan_scene.hpp>

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp> // IWYU pragma: keep

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace vkrndr
{
    struct vulkan_device;
    struct vulkan_pipeline;
    class vulkan_renderer;
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
            vkrndr::vulkan_renderer* renderer);

        void detach_renderer();

        void begin_frame();

        void end_frame();

        void update();

    public: // vulkan_scene overrides
        [[nodiscard]] VkClearValue clear_color() override;

        [[nodiscard]] VkClearValue clear_depth() override;

        [[nodiscard]] vkrndr::vulkan_image* depth_image() override;

        void resize(VkExtent2D extent) override;

        void draw(VkCommandBuffer command_buffer, VkExtent2D extent) override;

        void draw_imgui() override;

    public: // Operators
        scene& operator=(scene const&) = delete;

        scene& operator=(scene&&) noexcept = delete;

    private: // Types
        struct [[nodiscard]] frame_data final
        {
            VkDescriptorSet descriptor_set_{VK_NULL_HANDLE};
            vkrndr::vulkan_buffer vertex_uniform_buffer_;
        };

    private: // Data
        vkrndr::vulkan_device* vulkan_device_{};
        vkrndr::vulkan_renderer* vulkan_renderer_{};

        vkrndr::vulkan_buffer vert_index_buffer_;
        vkrndr::vulkan_image depth_buffer_;

        VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
        std::unique_ptr<vkrndr::vulkan_pipeline> pipeline_;

        std::vector<frame_data> frame_data_;

        uint32_t current_frame_{};
        size_t vertex_count_{};
        size_t index_count_{};
        glm::fmat4 local_matrix_{};
    };
} // namespace pawn

#endif
