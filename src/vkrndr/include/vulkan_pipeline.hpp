#ifndef VKRNDR_VULKAN_PIPELINE_INCLUDED
#define VKRNDR_VULKAN_PIPELINE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace vkrndr
{
    struct vulkan_device;
} // namespace vkrndr

namespace vkrndr
{
    struct [[nodiscard]] vulkan_pipeline final
    {
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
    };

    void destroy(vulkan_device* device, vulkan_pipeline* pipeline);

    class [[nodiscard]] vulkan_pipeline_builder final
    {
    public: // Construction
        vulkan_pipeline_builder(vulkan_device* device, VkFormat image_format);

        vulkan_pipeline_builder(vulkan_pipeline_builder const&) = delete;

        vulkan_pipeline_builder(vulkan_pipeline_builder&&) noexcept = delete;

    public: // Destruction
        ~vulkan_pipeline_builder();

    public: // Interface
        [[nodiscard]] vulkan_pipeline build();

        vulkan_pipeline_builder& add_shader(VkShaderStageFlagBits stage,
            std::filesystem::path const& path,
            std::string_view entry_point);

        vulkan_pipeline_builder& add_vertex_input(
            std::span<VkVertexInputBindingDescription const>
                binding_descriptions,
            std::span<VkVertexInputAttributeDescription const>
                attribute_descriptions);

        vulkan_pipeline_builder& add_descriptor_set_layout(
            VkDescriptorSetLayout descriptor_set_layout);

        vulkan_pipeline_builder& with_rasterization_samples(
            VkSampleCountFlagBits samples);

        vulkan_pipeline_builder& with_push_constants(
            VkPushConstantRange push_constants);

        vulkan_pipeline_builder& with_culling(VkCullModeFlags cull_mode,
            VkFrontFace front_face);

        vulkan_pipeline_builder& with_depth_test(VkFormat depth_format);

        vulkan_pipeline_builder& with_stencil_test(VkFormat depth_format,
            VkStencilOpState front,
            VkStencilOpState back);

    public: // Operators
        vulkan_pipeline_builder& operator=(
            vulkan_pipeline_builder const&) = delete;

        vulkan_pipeline_builder& operator=(
            vulkan_pipeline_builder&&) noexcept = delete;

    private: // Helpers
        void cleanup();

    private: // Data
        vulkan_device* device_{};
        VkFormat image_format_{};
        std::vector<
            std::tuple<VkShaderStageFlagBits, VkShaderModule, std::string>>
            shaders_;
        std::vector<VkVertexInputBindingDescription> vertex_input_binding_;
        std::vector<VkVertexInputAttributeDescription> vertex_input_attributes_;
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
        VkSampleCountFlagBits rasterization_samples_{VK_SAMPLE_COUNT_1_BIT};
        VkCullModeFlags cull_mode_{VK_CULL_MODE_NONE};
        VkFrontFace front_face_{VK_FRONT_FACE_COUNTER_CLOCKWISE};
        std::optional<VkPushConstantRange> push_constants_;
        VkFormat depth_format_{VK_FORMAT_UNDEFINED};
        std::optional<VkPipelineDepthStencilStateCreateInfo> depth_stencil_;
    };
} // namespace vkrndr

#endif // !VKRNDR_VULKAN_PIPELINE_INCLUDED
