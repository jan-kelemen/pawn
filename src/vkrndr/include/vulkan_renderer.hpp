#ifndef VKRNDR_VULKAN_RENDERER_INCLUDED
#define VKRNDR_VULKAN_RENDERER_INCLUDED

#include <vulkan/vulkan_core.h>

#include <vulkan_font.hpp>
#include <vulkan_image.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace vkrndr
{
    class font_manager;

    class imgui_render_layer;
    struct vulkan_context;
    struct vulkan_device;
    class vulkan_scene;
    class vulkan_swap_chain;
    class vulkan_window;
} // namespace vkrndr

namespace vkrndr
{
    class [[nodiscard]] vulkan_renderer final
    {
    public: // Construction
        vulkan_renderer(vulkan_window* window,
            vulkan_context* context,
            vulkan_device* device,
            vulkan_swap_chain* swap_chain);

        vulkan_renderer(vulkan_renderer const&) = delete;

        vulkan_renderer(vulkan_renderer&&) noexcept = delete;

    public: // Destruction
        ~vulkan_renderer();

    public: // Interface
        [[nodiscard]] constexpr VkDescriptorPool
        descriptor_pool() const noexcept;

        void set_imgui_layer(bool state);

        void begin_frame();

        void end_frame();

        void draw(vulkan_scene* scene);

        [[nodiscard]] vulkan_image load_texture(
            std::filesystem::path const& texture_path);

        [[nodiscard]] vulkan_font
        load_font(std::filesystem::path const& font_path, uint32_t font_size);

        void recreate();

    public: // Operators
        vulkan_renderer& operator=(vulkan_renderer const&) = delete;

        vulkan_renderer& operator=(vulkan_renderer&&) noexcept = delete;

    private: // Helpers
        void record_command_buffer(vulkan_scene* scene,
            VkCommandBuffer& command_buffer);

        [[nodiscard]] bool is_multisampled() const;

        void cleanup_images();

    private: // Data
        vulkan_window* window_;
        vulkan_context* context_;
        vulkan_device* device_;
        vulkan_swap_chain* swap_chain_;

        std::vector<VkCommandBuffer> command_buffers_;
        std::vector<VkCommandBuffer> secondary_buffers_;

        VkDescriptorPool descriptor_pool_{};

        vulkan_image color_image_;

        std::unique_ptr<imgui_render_layer> imgui_layer_;
        std::unique_ptr<font_manager> font_manager_;

        uint32_t current_frame_{};
    };
} // namespace vkrndr

constexpr VkDescriptorPool
vkrndr::vulkan_renderer::descriptor_pool() const noexcept
{
    return descriptor_pool_;
}

#endif // !VKRNDR_VULKAN_RENDERER_INCLUDED
