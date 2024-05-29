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

#include <cppext_numeric.hpp>
#include <cppext_pragma_warning.hpp>

#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>

// IWYU pragma: no_include <glm/detail/func_trigonometric.inl>
// IWYU pragma: no_include <glm/detail/qualifier.hpp>
// IWYU pragma: no_include <filesystem>
// IWYU pragma: no_include <functional>

namespace
{
    struct [[nodiscard]] vertex final
    {
        glm::fvec4 position;
        glm::fvec3 normal;
    };

    struct [[nodiscard]] transform final
    {
        glm::fmat4 model;
        glm::fmat4 view;
        glm::fmat4 projection;
    };

    DISABLE_WARNING_PUSH
    DISABLE_STRUCTURE_WAS_PADDED_DUE_TO_ALIGNMENT_SPECIFIER

    struct [[nodiscard]] push_constants final
    {
        alignas(16) glm::fvec3 color;
        alignas(16) glm::fvec3 camera;
        alignas(16) glm::fvec3 light_position;
        alignas(16) glm::fvec3 light_color;
        int transform_index;
    };

    DISABLE_WARNING_POP

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
                .offset = offsetof(vertex, position)},
            VkVertexInputAttributeDescription{.location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex, normal)}};

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
            .with_push_constants({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(push_constants)})
            .with_depth_stencil(depth_buffer_.format)
            .build());

    vkrndr::gltf_model const model{renderer->load_model("chess_set_2k.gltf")};

    for (auto const& node : model.nodes)
    {
        if (node.mesh)
        {
            auto const& current_mesh{meshes_.emplace_back(
                cppext::narrow<int32_t>(vertex_count_),
                vkrndr::count_cast(node.mesh->primitives[0].vertices.size()),
                cppext::narrow<int32_t>(index_count_),
                vkrndr::count_cast(node.mesh->primitives[0].indices.size()),
                local_matrix(node))};

            vertex_count_ += current_mesh.vertex_count;
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

    for (auto const& node : model.nodes)
    {
        if (node.mesh)
        {
            vertices = std::ranges::transform(node.mesh->primitives[0].vertices,
                vertices,
                [&](vkrndr::gltf_vertex const& vert)
                {
                    return vertex{.position = glm::fvec4(vert.position, 0.0f),
                        .normal = vert.normal};
                }).out;

            indices =
                std::ranges::copy(node.mesh->primitives[0].indices, indices)
                    .out;
        }
    }

    unmap_memory(vulkan_device_, &vert_index_map);

    frame_data_.resize(renderer->image_count());
    for (frame_data& data : frame_data_)
    {
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

        std::ranges::transform(meshes_,
            uniform_map.as<transform>(),
            [&](auto const& mesh)
            {
                // https://computergraphics.stackexchange.com/a/13809
                return transform{.model = mesh.local_matrix,
                    .view = glm::lookAt(camera_,
                        camera_ + front_face_,
                        up_direction_),
                    .projection = glm::orthoRH_ZO(camera_.x - projection_[0],
                        camera_.x + projection_[0],
                        camera_.y - projection_[0],
                        camera_.y + projection_[0],
                        camera_.z + projection_[1],
                        camera_.z + projection_[2])};
            });

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

    uint32_t const effective_dimension{std::min(extent.width, extent.height)};
    float const half_width{
        cppext::as_fp(extent.width - effective_dimension) / 2};
    float const half_height{
        cppext::as_fp(extent.height - effective_dimension) / 2};

    VkViewport const viewport{.x = half_width,
        .y = half_height,
        .width = cppext::as_fp(effective_dimension),
        .height = cppext::as_fp(effective_dimension),
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

    auto generate_color = [](auto index)
    {
        if (index == 6)
        {
            return glm::fvec4{0.5f, 0.8f, 0.4f, 1.0f};
        }

        return index % 2 ? glm::fvec4{0.2f, 0.2f, 0.2f, 1.0f}
                         : glm::fvec4{0.8f, 0.8f, 0.8f, 1.0f};
    };

    for (auto const& [transform_index, mesh] : std::views::enumerate(meshes_))
    {
        push_constants const constants{.color = generate_color(transform_index),
            .camera = camera_,
            .light_position = light_position_,
            .light_color = light_color_,
            .transform_index = cppext::narrow<int>(transform_index)};

        vkCmdPushConstants(command_buffer,
            pipeline_->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(push_constants),
            &constants);

        vkCmdDrawIndexed(command_buffer,
            mesh.index_count,
            1,
            vkrndr::count_cast(mesh.index_offset),
            mesh.vertex_offset,
            0);
    }
}

void pawn::scene::draw_imgui()
{
    ImGui::Begin("Camera");
    ImGui::SliderFloat3("Camera", glm::value_ptr(camera_), -10.f, 10.f);
    ImGui::SliderFloat3("Front face", glm::value_ptr(front_face_), -10.f, 10.f);
    ImGui::SliderFloat3("Up direction",
        glm::value_ptr(up_direction_),
        -10.f,
        10.f);
    ImGui::End();

    ImGui::Begin("Projection");
    // NOLINTNEXTLINE(readability-container-data-pointer)
    ImGui::SliderFloat("Zoom", &projection_[0], 0, 10.f);
    ImGui::SliderFloat("Near", &projection_[1], -10.f, 10.f);
    ImGui::SliderFloat("Far", &projection_[2], -10.f, 10.f);
    ImGui::End();

    ImGui::Begin("Eye");
    ImGui::Value("Eye x", camera_.x);
    ImGui::Value("Eye y", camera_.y);
    ImGui::Value("Eye z", camera_.z);
    ImGui::End();

    ImGui::Begin("Center");
    ImGui::Value("Center x", (camera_ + front_face_).x);
    ImGui::Value("Center y", (camera_ + front_face_).y);
    ImGui::Value("Center z", (camera_ + front_face_).z);
    ImGui::End();

    ImGui::Begin("Light");
    ImGui::SliderFloat3("Position",
        glm::value_ptr(light_position_),
        -1.0f,
        1.0f);
    ImGui::SliderFloat3("Color", glm::value_ptr(light_color_), 0.0f, 1.0f);
    ImGui::End();
}
