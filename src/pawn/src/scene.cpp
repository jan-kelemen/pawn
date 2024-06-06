#include <scene.hpp>
#include <uci_engine.hpp>

#include <gltf_manager.hpp>
#include <utility>
#include <vulkan_buffer.hpp>
#include <vulkan_depth_buffer.hpp>
#include <vulkan_descriptors.hpp>
#include <vulkan_device.hpp>
#include <vulkan_image.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_utility.hpp>

#include <cppext_attribute.hpp>
#include <cppext_numeric.hpp>
#include <cppext_pragma_warning.hpp>

#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp> // IWYU pragma: keep
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>

// IWYU pragma: no_include <glm/detail/func_trigonometric.inl>
// IWYU pragma: no_include <glm/detail/qualifier.hpp>
// IWYU pragma: no_include <filesystem>
// IWYU pragma: no_include <functional>

namespace
{
    DISABLE_WARNING_PUSH
    DISABLE_WARNING_STRUCTURE_WAS_PADDED_DUE_TO_ALIGNMENT_SPECIFIER

    struct [[nodiscard]] vertex final
    {
        glm::fvec4 position;
        alignas(16) glm::fvec3 normal;
        alignas(16) glm::fvec2 texture_coordinates;
    };

    DISABLE_WARNING_POP

    struct [[nodiscard]] transform final
    {
        glm::fmat4 model;
        glm::fmat4 view;
        glm::fmat4 projection;
    };

    DISABLE_WARNING_PUSH
    DISABLE_WARNING_STRUCTURE_WAS_PADDED_DUE_TO_ALIGNMENT_SPECIFIER

    struct [[nodiscard]] push_constants final
    {
        alignas(16) glm::fvec3 color;
        alignas(16) glm::fvec3 camera;
        alignas(16) glm::fvec3 light_position;
        alignas(16) glm::fvec3 light_color;
        int transform_index;
        float outline_width;
        uint32_t use_texture;
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
                .offset = offsetof(vertex, normal)},
            VkVertexInputAttributeDescription{.location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vertex, texture_coordinates)}};

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

        VkDescriptorSetLayoutBinding sampler_layout_binding{};
        sampler_layout_binding.binding = 1;
        sampler_layout_binding.descriptorCount = 1;
        sampler_layout_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        sampler_layout_binding.pImmutableSamplers = nullptr;

        std::array const bindings{vertex_uniform_binding,
            sampler_layout_binding};

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
        VkDescriptorBufferInfo const vertex_buffer_info,
        VkSampler const texture_sampler,
        VkImageView const texture_image_view)
    {
        VkWriteDescriptorSet vertex_descriptor_write{};
        vertex_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertex_descriptor_write.dstSet = descriptor_set;
        vertex_descriptor_write.dstBinding = 0;
        vertex_descriptor_write.dstArrayElement = 0;
        vertex_descriptor_write.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vertex_descriptor_write.descriptorCount = 1;
        vertex_descriptor_write.pBufferInfo = &vertex_buffer_info;

        VkDescriptorImageInfo texture_image_info{};
        texture_image_info.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture_image_info.imageView = texture_image_view;
        texture_image_info.sampler = texture_sampler;

        VkWriteDescriptorSet texture_descriptor_write{};
        texture_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texture_descriptor_write.dstSet = descriptor_set;
        texture_descriptor_write.dstBinding = 1;
        texture_descriptor_write.dstArrayElement = 0;
        texture_descriptor_write.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_descriptor_write.descriptorCount = 1;
        texture_descriptor_write.pImageInfo = &texture_image_info;

        std::array const descriptor_writes{vertex_descriptor_write,
            texture_descriptor_write};

        vkUpdateDescriptorSets(device->logical,
            vkrndr::count_cast(descriptor_writes.size()),
            descriptor_writes.data(),
            0,
            nullptr);
    }

    [[nodiscard]] VkSampler create_texture_sampler(
        vkrndr::vulkan_device const* const device,
        uint32_t const mip_levels = 1)
    {
        VkPhysicalDeviceProperties properties; // NOLINT
        vkGetPhysicalDeviceProperties(device->physical, &properties);

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = static_cast<float>(mip_levels);

        VkSampler rv; // NOLINT
        vkrndr::check_result(
            vkCreateSampler(device->logical, &sampler_info, nullptr, &rv));

        return rv;
    }

    [[nodiscard]] constexpr glm::fvec3 calculate_position(
        pawn::board_piece piece)
    {
        constexpr float center{0.028944039717316628};
        return glm::fvec3{center + (float(piece.column) - 4) * center * 2,
            0,
            center + (float(piece.row) - 4) * center * 2};
    }
} // namespace

pawn::scene::scene(uci_engine const& engine) : engine_{&engine} {};

pawn::scene::~scene() = default;

void pawn::scene::attach_renderer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_renderer* renderer)
{
    vulkan_device_ = device;
    vulkan_renderer_ = renderer;

    descriptor_set_layout_ = create_descriptor_set_layout(vulkan_device_);

    depth_buffer_ =
        vkrndr::create_depth_buffer(device, renderer->extent(), true);

    VkStencilOpState const default_stencil_state{.failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = 0xff};
    piece_pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
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
            .with_depth_test(depth_buffer_.format)
            .with_stencil_test(depth_buffer_.format,
                default_stencil_state,
                default_stencil_state)
            .build());

    VkStencilOpState const model_stencil_state{.failOp = VK_STENCIL_OP_REPLACE,
        .passOp = VK_STENCIL_OP_REPLACE,
        .depthFailOp = VK_STENCIL_OP_REPLACE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = 0x1};
    outlined_piece_pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
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
            .with_depth_test(depth_buffer_.format)
            .with_stencil_test(depth_buffer_.format,
                model_stencil_state,
                model_stencil_state)
            .build());

    VkStencilOpState const outline_stencil_state{.failOp = VK_STENCIL_OP_KEEP,
        .passOp = VK_STENCIL_OP_REPLACE,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp = VK_COMPARE_OP_NOT_EQUAL,
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = 1};
    outline_pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
        vkrndr::vulkan_pipeline_builder{vulkan_device_,
            renderer->image_format()}
            .add_shader(VK_SHADER_STAGE_VERTEX_BIT, "outline.vert.spv", "main")
            .add_shader(VK_SHADER_STAGE_FRAGMENT_BIT,
                "outline.frag.spv",
                "main")
            .with_rasterization_samples(vulkan_device_->max_msaa_samples)
            .add_vertex_input(binding_description(), attribute_descriptions())
            .add_descriptor_set_layout(descriptor_set_layout_)
            .with_push_constants({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(push_constants)})
            .with_stencil_test(depth_buffer_.format,
                outline_stencil_state,
                outline_stencil_state)
            .with_color_blending(VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT,
            })
            .build());

    using namespace std::string_view_literals;
    constexpr std::array load_nodes{
        std::pair{piece_type::rook, "piece_rook_white_01"sv},
        std::pair{piece_type::pawn, "piece_pawn_white_01"sv},
        std::pair{piece_type::bishop, "piece_bishop_white_01"sv},
        std::pair{piece_type::queen, "piece_queen_white"sv},
        std::pair{piece_type::king, "piece_king_white"sv},
        std::pair{piece_type::knight, "piece_knight_white_01"sv},
        std::pair{piece_type::none, "board"sv}};

    auto model{renderer->load_model("chess_set_2k.gltf")};

    std::vector<vkrndr::gltf_mesh const*> load_meshes;
    load_meshes.reserve(load_nodes.size());

    for (auto const& [type, name] : load_nodes)
    {
        auto const it{std::ranges::find_if(
            model->nodes,
            [&name](auto const& node_name)
            { CPPEXT_SUPPRESS return name == node_name; },
            &vkrndr::gltf_node::name)};
        if (it == std::cend(model->nodes))
        {
            continue;
        }

        auto const& model_mesh{*it->mesh};
        load_meshes.push_back(&model_mesh);

        auto const& current_mesh{meshes_.emplace_back(type,
            cppext::narrow<int32_t>(vertex_count_),
            vkrndr::count_cast(model_mesh.primitives[0].vertices.size()),
            cppext::narrow<int32_t>(index_count_),
            vkrndr::count_cast(model_mesh.primitives[0].indices.size()),
            local_matrix(*it))};

        vertex_count_ += current_mesh.vertex_count;
        index_count_ += current_mesh.index_count;
    }

    size_t const vertices_size{vertex_count_ * sizeof(vertex)};
    size_t const indices_size{index_count_ * sizeof(uint32_t)};

    vkrndr::vulkan_buffer staging_buffer{create_buffer(vulkan_device_,
        vertices_size + indices_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    {
        vkrndr::mapped_memory vert_index_map{vkrndr::map_memory(vulkan_device_,
            staging_buffer.memory,
            vertices_size + indices_size)};

        vertex* vertices{vert_index_map.as<vertex>()};
        uint32_t* indices{vert_index_map.as<uint32_t>(vertices_size)};

        for (auto const& mesh : load_meshes)
        {
            vertices = std::ranges::transform(mesh->primitives[0].vertices,
                vertices,
                [&](vkrndr::gltf_vertex const& vert)
                {
                    return vertex{.position = glm::fvec4(vert.position, 0.0f),
                        .normal = vert.normal,
                        .texture_coordinates = vert.texture_coordinate};
                }).out;

            indices =
                std::ranges::copy(mesh->primitives[0].indices, indices).out;
        }

        unmap_memory(vulkan_device_, &vert_index_map);
    }

    vert_index_buffer_ = create_buffer(vulkan_device_,
        vertices_size + indices_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vulkan_renderer_->transfer_buffer(staging_buffer, vert_index_buffer_);
    destroy(vulkan_device_, &staging_buffer);

    size_t const uniform_buffer_size{sizeof(transform) * draw_meshes_.size()};
    vertex_uniform_buffer_ = create_buffer(vulkan_device_,
        uniform_buffer_size * renderer->image_count(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    texture_sampler_ = create_texture_sampler(vulkan_device_);
    texture_image_ = model->textures[4].image;

    frame_data_.resize(renderer->image_count());
    for (auto const& [index, data] : std::views::enumerate(frame_data_))
    {
        create_descriptor_sets(vulkan_device_,
            descriptor_set_layout_,
            renderer->descriptor_pool(),
            std::span{&data.descriptor_set_, 1});

        bind_descriptor_set(vulkan_device_,
            data.descriptor_set_,
            VkDescriptorBufferInfo{.buffer = vertex_uniform_buffer_.buffer,
                .offset =
                    cppext::narrow<VkDeviceSize>(index) * uniform_buffer_size,
                .range = uniform_buffer_size},
            texture_sampler_,
            texture_image_.view);
    }

    draw_meshes_[0] = to_board_peice(0, 0, mesh_color::none, piece_type::none);

    for (auto [index, texture] : model->textures | std::views::enumerate)
    {
        if (index != 4)
        {
            destroy(vulkan_device_, &texture.image);
        }
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
        }

        destroy(vulkan_device_, &texture_image_);
        vkDestroySampler(vulkan_device_->logical, texture_sampler_, nullptr);

        destroy(vulkan_device_, outlined_piece_pipeline_.get());
        outlined_piece_pipeline_.reset();

        destroy(vulkan_device_, outline_pipeline_.get());
        outline_pipeline_.reset();

        destroy(vulkan_device_, piece_pipeline_.get());
        piece_pipeline_.reset();

        vkDestroyDescriptorSetLayout(vulkan_device_->logical,
            descriptor_set_layout_,
            nullptr);

        destroy(vulkan_device_, &depth_buffer_);
        destroy(vulkan_device_, &vertex_uniform_buffer_);
        destroy(vulkan_device_, &vert_index_buffer_);
    }
    vulkan_device_ = nullptr;
}

void pawn::scene::begin_frame()
{
    used_pieces_ = 1;
    current_frame_ = (current_frame_ + 1) % vulkan_renderer_->image_count();
}

void pawn::scene::end_frame() { }

void pawn::scene::add_piece(board_piece piece)
{
    draw_meshes_[used_pieces_++] = piece;
}

void pawn::scene::update()
{
    {
        vkrndr::mapped_memory uniform_map{vkrndr::map_memory(vulkan_device_,
            vertex_uniform_buffer_.memory,
            sizeof(transform) * draw_meshes_.size(),
            current_frame_ * sizeof(transform) * draw_meshes_.size())};

        auto const view_matrix{
            glm::lookAt(camera_, camera_ + front_face_, up_direction_)};

        // https://computergraphics.stackexchange.com/a/13809
        auto const projection_matrix{glm::orthoRH_ZO(camera_.x - projection_[0],
            camera_.x + projection_[0],
            camera_.y - projection_[0],
            camera_.y + projection_[0],
            camera_.z + projection_[1],
            camera_.z + projection_[2])};

        std::ranges::transform(draw_meshes_ | std::views::take(used_pieces_),
            uniform_map.as<transform>(),
            [&, this](auto const& draw_mesh)
            {
                auto const it{std::ranges::find_if(
                    meshes_,
                    [&draw_mesh](auto const type)
                    { return type == draw_mesh.type; },
                    &mesh::type)};
                assert(it != std::cend(meshes_));

                auto const position{calculate_position(draw_mesh)};
                auto model_matrix{it->local_matrix};

                // Override translation component to real board position
                if (draw_mesh.type != piece_type::none)
                {
                    model_matrix[3][0] = position.x;
                    model_matrix[3][2] = position.z;
                }

                // Rotate black pieces toward center of board
                if (draw_mesh.color == std::to_underlying(mesh_color::black))
                {
                    model_matrix = glm::rotate(model_matrix,
                        glm::radians(180.0f),
                        glm::fvec3(0, 1, 0));
                }

                return transform{.model = model_matrix,
                    .view = view_matrix,
                    .projection = projection_matrix};
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
    depth_buffer_ = vkrndr::create_depth_buffer(vulkan_device_, extent, true);
}

void pawn::scene::draw(VkCommandBuffer command_buffer, VkExtent2D extent)
{
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
        outlined_piece_pipeline_->pipeline_layout,
        0,
        1,
        &frame_data_[current_frame_].descriptor_set_,
        0,
        nullptr);

    auto const generate_color = [](board_piece draw_mesh)
    {
        return (draw_mesh.color == std::to_underlying(mesh_color::black))
            ? glm::fvec4{0.1f, 0.1f, 0.1f, 1.0f}
            : glm::fvec4{0.9f, 0.9f, 0.9f, 1.0f};
    };

    for (auto const& [transform_index, draw_mesh] :
        draw_meshes_ | std::views::take(used_pieces_) | std::views::enumerate)
    {
        if (draw_mesh.type != piece_type::king)
        {
            vkCmdBindPipeline(command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                piece_pipeline_->pipeline);
        }
        else
        {
            vkCmdBindPipeline(command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                outlined_piece_pipeline_->pipeline);
        }
        push_constants const constants{.color = generate_color(draw_mesh),
            .camera = camera_,
            .light_position = light_position_,
            .light_color = light_color_,
            .transform_index = cppext::narrow<int>(transform_index),
            .outline_width = 0.0f,
            .use_texture = transform_index == 0};

        vkCmdPushConstants(command_buffer,
            outlined_piece_pipeline_->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(push_constants),
            &constants);

        auto const it{std::ranges::find_if(
            meshes_,
            [&draw_mesh](auto const& type)
            { CPPEXT_SUPPRESS return draw_mesh.type == type; },
            &mesh::type)};
        assert(it != std::cend(meshes_));

        vkCmdDrawIndexed(command_buffer,
            it->index_count,
            1,
            vkrndr::count_cast(it->index_offset),
            it->vertex_offset,
            0);
    }

    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        outline_pipeline_->pipeline);
    for (auto const& [transform_index, draw_mesh] :
        draw_meshes_ | std::views::take(used_pieces_) | std::views::enumerate)
    {
        if (draw_mesh.type != piece_type::king)
        {
            continue;
        }

        push_constants const constants{.color = generate_color(draw_mesh),
            .camera = camera_,
            .light_position = light_position_,
            .light_color = light_color_,
            .transform_index = cppext::narrow<int>(transform_index),
            .outline_width = 0.0025f,
            .use_texture = transform_index == 0};

        vkCmdPushConstants(command_buffer,
            outlined_piece_pipeline_->pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(push_constants),
            &constants);

        auto const it{std::ranges::find_if(
            meshes_,
            [&draw_mesh](auto const& type)
            { CPPEXT_SUPPRESS return draw_mesh.type == type; },
            &mesh::type)};
        assert(it != std::cend(meshes_));

        vkCmdDrawIndexed(command_buffer,
            it->index_count,
            1,
            vkrndr::count_cast(it->index_offset),
            it->vertex_offset,
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

    ImGui::Begin("Engine debug");
    for (std::string const& line : engine_->debug_output())
    {
        ImGui::Text(line.c_str());
    }
    ImGui::End();
}
