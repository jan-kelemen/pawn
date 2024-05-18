#include <scene.hpp>

#include <vulkan_buffer.hpp>
#include <vulkan_device.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_swap_chain.hpp>

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>

// IWYU pragma: no_include <filesystem>
// IWYU pragma: no_include <span>

namespace
{
    struct [[nodiscard]] vertex final
    {
        glm::fvec3 position;
    };

    consteval auto binding_description()
    {
        constexpr std::array descriptions{
            VkVertexInputBindingDescription{.binding = 0,
                .stride = sizeof(vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        };

        return descriptions;
    }

    consteval auto attribute_descriptions()
    {
        constexpr std::array descriptions{
            VkVertexInputAttributeDescription{.location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex, position)}};

        return descriptions;
    }
} // namespace

pawn::scene::scene() = default;

pawn::scene::~scene() = default;

void pawn::scene::attach_renderer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_swap_chain const* swap_chain,
    [[maybe_unused]] vkrndr::vulkan_renderer* renderer)
{
    vulkan_device_ = device;

    pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
        vkrndr::vulkan_pipeline_builder{vulkan_device_,
            swap_chain->image_format()}
            .add_shader(VK_SHADER_STAGE_VERTEX_BIT, "scene.vert.spv", "main")
            .add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, "scene.frag.spv", "main")
            .with_rasterization_samples(vulkan_device_->max_msaa_samples)
            .add_vertex_input(binding_description(), attribute_descriptions())
            .build());

    size_t const vertices_size{4 * sizeof(vertex)};
    size_t const indices_size{6 * sizeof(uint16_t)};

    vert_index_buffer_ = create_buffer(vulkan_device_,
        vertices_size + indices_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        vkrndr::mapped_memory vert_index_map{vkrndr::map_memory(vulkan_device_,
            vert_index_buffer_.memory,
            vertices_size + indices_size)};

        vertex* const vertices{vert_index_map.as<vertex>(0)};
        uint16_t* const indices{vert_index_map.as<uint16_t>(vertices_size)};

        vertices[0] = {{0.f, 0.f, 0}};
        vertices[1] = {{1.f, 0.f, 0}};
        vertices[2] = {{1.f, 1.f, 0}};
        vertices[3] = {{0.f, 1.f, 0}};

        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 2;
        indices[4] = 3;
        indices[5] = 0;

        unmap_memory(vulkan_device_, &vert_index_map);
    }
}

void pawn::scene::detach_renderer()
{
    if (vulkan_device_)
    {
        destroy(vulkan_device_, pipeline_.release());

        destroy(vulkan_device_, &vert_index_buffer_);
    }
    vulkan_device_ = nullptr;
}

VkClearValue pawn::scene::clear_color() { return {{{1.f, .5f, .3f, 1.f}}}; }

void pawn::scene::draw(VkCommandBuffer command_buffer, VkExtent2D extent)
{
    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline);

    size_t const index_offset{4 * sizeof(vertex)};
    VkDeviceSize const zero_offsets{0};
    vkCmdBindVertexBuffers(command_buffer,
        0,
        1,
        &vert_index_buffer_.buffer,
        &zero_offsets);
    vkCmdBindIndexBuffer(command_buffer,
        vert_index_buffer_.buffer,
        index_offset,
        VK_INDEX_TYPE_UINT16);

    VkViewport const viewport{.x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D const scissor{{0, 0}, extent};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);
}

void pawn::scene::draw_imgui() { }
