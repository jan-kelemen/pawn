#include <scene.hpp>

#include <gltf_manager.hpp>
#include <vulkan_buffer.hpp>
#include <vulkan_depth_buffer.hpp>
#include <vulkan_descriptors.hpp>
#include <vulkan_device.hpp>
#include <vulkan_image.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_utility.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>

// IWYU pragma: no_include <glm/detail/func_trigonometric.inl>
// IWYU pragma: no_include <glm/detail/qualifier.hpp>
// IWYU pragma: no_include <filesystem>

namespace
{
    struct [[nodiscard]] vertex final
    {
        glm::fvec4 position;
    };

    struct [[nodiscard]] transform final
    {
        glm::fmat4 model;
        glm::fmat4 view;
        glm::fmat4 projection;
    };

    struct [[nodiscard]] push_constants final
    {
        int transform_index;
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
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(vertex, position)}};

        return descriptions;
    }

    [[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(
        vkrndr::vulkan_device const* const device)
    {
        VkDescriptorSetLayoutBinding vertex_uniform_binding{};
        vertex_uniform_binding.binding = 0;
        vertex_uniform_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertex_uniform_binding.descriptorCount = 1;
        vertex_uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::array const bindings{vertex_uniform_binding};

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = vkrndr::count_cast(bindings.size());
        layout_info.pBindings = bindings.data();

        VkDescriptorSetLayout rv; // NOLINT
        vkrndr::check_result(vkCreateDescriptorSetLayout(device->logical,
            &layout_info,
            nullptr,
            &rv));

        return rv;
    }

    void bind_descriptor_set(vkrndr::vulkan_device const* const device,
        VkDescriptorSet const& descriptor_set,
        VkBuffer const vertex_buffer)
    {
        VkDescriptorBufferInfo const vertex_buffer_info{.buffer = vertex_buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE};

        VkWriteDescriptorSet vertex_descriptor_write{};
        vertex_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertex_descriptor_write.dstSet = descriptor_set;
        vertex_descriptor_write.dstBinding = 0;
        vertex_descriptor_write.dstArrayElement = 0;
        vertex_descriptor_write.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertex_descriptor_write.descriptorCount = 1;
        vertex_descriptor_write.pBufferInfo = &vertex_buffer_info;

        std::array const descriptor_writes{vertex_descriptor_write};

        vkUpdateDescriptorSets(device->logical,
            vkrndr::count_cast(descriptor_writes.size()),
            descriptor_writes.data(),
            0,
            nullptr);
    }
} // namespace

pawn::scene::scene() = default;

pawn::scene::~scene() = default;

void pawn::scene::attach_renderer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_renderer* renderer)
{
    vulkan_device_ = device;
    vulkan_renderer_ = renderer;

    descriptor_set_layout_ = create_descriptor_set_layout(vulkan_device_);

    depth_buffer_ = vkrndr::create_depth_buffer(device, renderer->extent());

    pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
        vkrndr::vulkan_pipeline_builder{vulkan_device_,
            renderer->image_format()}
            .add_shader(VK_SHADER_STAGE_VERTEX_BIT, "scene.vert.spv", "main")
            .add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, "scene.frag.spv", "main")
            .with_rasterization_samples(vulkan_device_->max_msaa_samples)
            .add_vertex_input(binding_description(), attribute_descriptions())
            .add_descriptor_set_layout(descriptor_set_layout_)
            .with_push_constants({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(push_constants)})
            .with_depth_stencil(depth_buffer_.format)
            .build());

    vkrndr::gltf_model model{renderer->load_model("chess_set_2k.gltf")};

    int32_t vertex_offset{};
    int32_t index_offset{};
    for (auto const& node : model.nodes)
    {
        if (node.mesh)
        {
            auto const& current_mesh{meshes_.emplace_back(vertex_offset,
                node.mesh->primitives[0].vertices.size(),
                index_offset,
                node.mesh->primitives[0].indices.size(),
                local_matrix(node))};

            vertex_offset += current_mesh.vertex_count;
            vertex_count_ += current_mesh.vertex_count;
            index_offset += current_mesh.index_count;
            index_count_ += current_mesh.index_count;
        }
    }

    size_t const vertices_size{vertex_count_ * sizeof(vertex)};
    size_t const indices_size{index_count_ * sizeof(uint32_t)};
    vert_index_buffer_ = create_buffer(vulkan_device_,
        vertices_size + indices_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkrndr::mapped_memory vert_index_map{vkrndr::map_memory(vulkan_device_,
        vert_index_buffer_.memory,
        vertices_size + indices_size)};

    vertex* vertices{vert_index_map.as<vertex>()};
    uint32_t* indices{vert_index_map.as<uint32_t>(vertices_size)};

    size_t i{};
    for (auto const& node : model.nodes)
    {
        if (node.mesh)
        {
            auto const& current_mesh{meshes_[i]};

            vertices = std::ranges::transform(
                node.mesh->primitives[0].vertices,
                vertices,
                [&i](glm::fvec3 const& vec) {
                    return vertex{
                        .position = glm::fvec4(vec, static_cast<float>(i))};
                },
                &vkrndr::gltf_vertex::position)
                           .out;

            indices =
                std::ranges::copy(node.mesh->primitives[0].indices, indices)
                    .out;

            ++i;
        }
    }

    unmap_memory(vulkan_device_, &vert_index_map);

    frame_data_.resize(renderer->image_count());
    for (uint32_t i{}; i != frame_data_.size(); ++i)
    {
        frame_data& data{frame_data_[i]};

        data.vertex_uniform_buffer_ = create_buffer(vulkan_device_,
            sizeof(transform) * meshes_.size(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        create_descriptor_sets(vulkan_device_,
            descriptor_set_layout_,
            renderer->descriptor_pool(),
            std::span{&data.descriptor_set_, 1});

        bind_descriptor_set(vulkan_device_,
            data.descriptor_set_,
            data.vertex_uniform_buffer_.buffer);
    }
}

void pawn::scene::detach_renderer()
{
    if (vulkan_device_)
    {
        for (auto& data : frame_data_)
        {
            vkFreeDescriptorSets(vulkan_device_->logical,
                vulkan_renderer_->descriptor_pool(),
                1,
                &data.descriptor_set_);

            destroy(vulkan_device_, &data.vertex_uniform_buffer_);
        }

        destroy(vulkan_device_, pipeline_.release());
        vkDestroyDescriptorSetLayout(vulkan_device_->logical,
            descriptor_set_layout_,
            nullptr);

        destroy(vulkan_device_, &depth_buffer_);

        destroy(vulkan_device_, &vert_index_buffer_);
    }
    vulkan_device_ = nullptr;
}

void pawn::scene::begin_frame()
{
    current_frame_ = (current_frame_ + 1) % vulkan_renderer_->image_count();
}

void pawn::scene::end_frame() { }

void pawn::scene::update()
{
    {
        vkrndr::mapped_memory uniform_map{vkrndr::map_memory(vulkan_device_,
            frame_data_[current_frame_].vertex_uniform_buffer_.memory,
            sizeof(transform))};

        constexpr glm::fvec3 front_face{0, 0, -1};
        constexpr glm::fvec3 up_direction{0, -1, 0};
        constexpr glm::fvec3 camera{0, 0, 0};

        auto transforms{std::span{uniform_map.as<transform>(), meshes_.size()}};

        for (auto const& [mesh, transform_object] :
            std::views::zip(meshes_, transforms))
        {
            transform const uniform{.model = mesh.local_matrix,
                .view = glm::lookAt(camera, camera + front_face, up_direction),
                .projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f)};

            transform_object = uniform;
        }

        unmap_memory(vulkan_device_, &uniform_map);
    }
}

VkClearValue pawn::scene::clear_color() { return {{{1.f, .5f, .3f, 1.f}}}; }

VkClearValue pawn::scene::clear_depth() { return {.depthStencil = {1.0f, 0}}; }

vkrndr::vulkan_image* pawn::scene::depth_image() { return &depth_buffer_; }

void pawn::scene::resize(VkExtent2D extent)
{
    destroy(vulkan_device_, &depth_buffer_);
    depth_buffer_ = vkrndr::create_depth_buffer(vulkan_device_, extent);
}

void pawn::scene::draw(VkCommandBuffer command_buffer, VkExtent2D extent)
{
    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline);

    size_t const index_offset{vertex_count_ * sizeof(vertex)};
    VkDeviceSize const zero_offsets{0};
    vkCmdBindVertexBuffers(command_buffer,
        0,
        1,
        &vert_index_buffer_.buffer,
        &zero_offsets);
    vkCmdBindIndexBuffer(command_buffer,
        vert_index_buffer_.buffer,
        index_offset,
        VK_INDEX_TYPE_UINT32);

    VkViewport const viewport{.x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D const scissor{{0, 0}, extent};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline_layout,
        0,
        1,
        &frame_data_[current_frame_].descriptor_set_,
        0,
        nullptr);

    for (int i{}; i != static_cast<int>(meshes_.size()); ++i)
    {
        push_constants pc{.transform_index = i};

        vkCmdPushConstants(command_buffer,
            pipeline_->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(push_constants),
            &pc);

        vkCmdDrawIndexed(command_buffer,
            meshes_[i].index_count,
            1,
            meshes_[i].index_offset,
            meshes_[i].vertex_offset,
            0);
    }
}

void pawn::scene::draw_imgui() { }
