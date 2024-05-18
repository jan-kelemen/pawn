#include <vulkan_renderer.hpp>

#include <font_manager.hpp>
#include <global_data.hpp>
#include <imgui_render_layer.hpp>
#include <vulkan_buffer.hpp>
#include <vulkan_commands.hpp>
#include <vulkan_device.hpp>
#include <vulkan_font.hpp>
#include <vulkan_image.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_queue.hpp>
#include <vulkan_scene.hpp>
#include <vulkan_swap_chain.hpp>
#include <vulkan_utility.hpp>
#include <vulkan_window.hpp>

#include <stb_image.h>

#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

// IWYU pragma: no_include <functional>

namespace
{
    VkDescriptorPool create_descriptor_pool(
        vkrndr::vulkan_device const* const device)
    {
        constexpr auto count{vkrndr::count_cast(
            vkrndr::vulkan_swap_chain::max_frames_in_flight)};

        VkDescriptorPoolSize uniform_buffer_pool_size{};
        uniform_buffer_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_pool_size.descriptorCount = 3 * count;

        VkDescriptorPoolSize texture_sampler_pool_size{};
        texture_sampler_pool_size.type =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_sampler_pool_size.descriptorCount = 2 * count;

        std::array pool_sizes{uniform_buffer_pool_size,
            texture_sampler_pool_size};

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.poolSizeCount = vkrndr::count_cast(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 4 * count + count;

        VkDescriptorPool rv{};
        vkrndr::check_result(
            vkCreateDescriptorPool(device->logical, &pool_info, nullptr, &rv));

        return rv;
    }

} // namespace

vkrndr::vulkan_renderer::vulkan_renderer(vulkan_window* const window,
    vulkan_context* const context,
    vulkan_device* const device,
    vulkan_swap_chain* const swap_chain)
    : window_{window}
    , context_{context}
    , device_{device}
    , swap_chain_{swap_chain}
    , command_buffers_{vulkan_swap_chain::max_frames_in_flight}
    , secondary_buffers_{vulkan_swap_chain::max_frames_in_flight}
    , descriptor_pool_{create_descriptor_pool(device)}
    , font_manager_{std::make_unique<font_manager>()}
{
    recreate();

    create_command_buffers(device_,
        device->present_queue->command_pool,
        vulkan_swap_chain::max_frames_in_flight,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        command_buffers_);

    create_command_buffers(device_,
        device->present_queue->command_pool,
        vulkan_swap_chain::max_frames_in_flight,
        VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        secondary_buffers_);
}

vkrndr::vulkan_renderer::~vulkan_renderer()
{
    imgui_layer_.reset();

    vkDestroyDescriptorPool(device_->logical, descriptor_pool_, nullptr);

    cleanup_images();
}

void vkrndr::vulkan_renderer::set_imgui_layer(bool state)
{
    if (imgui_layer_)
    {
        imgui_layer_.reset();
    }

    if (state)
    {
        imgui_layer_ = std::make_unique<imgui_render_layer>(window_,
            context_,
            device_,
            swap_chain_);
    }
}

void vkrndr::vulkan_renderer::begin_frame()
{
    if (imgui_layer_)
    {
        imgui_layer_->begin_frame();
    }
}

void vkrndr::vulkan_renderer::end_frame()
{
    if (imgui_layer_)
    {
        imgui_layer_->end_frame();
    }
}

void vkrndr::vulkan_renderer::draw(vulkan_scene* scene)
{
    if (swap_chain_refresh.load())
    {
        if (window_->is_minimized())
        {
            return;
        }

        vkDeviceWaitIdle(device_->logical);
        swap_chain_->recreate();
        recreate();
        swap_chain_refresh.store(false);
        return;
    }

    uint32_t image_index{};
    if (!swap_chain_->acquire_next_image(current_frame_, image_index))
    {
        return;
    }

    auto& command_buffer{command_buffers_[current_frame_]};

    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    check_result(vkBeginCommandBuffer(command_buffer, &begin_info));

    transition_image(swap_chain_->image(image_index),
        command_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment_info{};
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.imageLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue = scene->clear_color();
    if (is_multisampled())
    {
        color_attachment_info.imageView = color_image_.view;
        color_attachment_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        color_attachment_info.resolveImageView =
            swap_chain_->image_view(image_index);
        color_attachment_info.resolveImageLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
        color_attachment_info.imageView = swap_chain_->image_view(image_index);
    }

    VkRenderingInfo render_info{};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    render_info.renderArea = {{0, 0}, swap_chain_->extent()};
    render_info.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment_info;

    vkCmdBeginRendering(command_buffer, &render_info);

    record_command_buffer(scene, secondary_buffers_[current_frame_]);
    vkCmdExecuteCommands(command_buffer,
        1,
        &secondary_buffers_[current_frame_]);

    vkCmdEndRendering(command_buffer);

    transition_image(swap_chain_->image(image_index),
        command_buffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    check_result(vkEndCommandBuffer(command_buffer));

    swap_chain_->submit_command_buffers(std::span{&command_buffer, 1},
        current_frame_,
        image_index);

    current_frame_ =
        (current_frame_ + 1) % vulkan_swap_chain::max_frames_in_flight;
}

vkrndr::vulkan_image vkrndr::vulkan_renderer::load_texture(
    std::filesystem::path const& texture_path)
{
    int width; // NOLINT
    int height; // NOLINT
    int channels; // NOLINT
    stbi_uc* const pixels{stbi_load(texture_path.string().c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha)};

    auto const image_size{static_cast<VkDeviceSize>(width * height * 4)};

    if (!pixels)
    {
        throw std::runtime_error{"failed to load texture image!"};
    }

    vulkan_buffer staging_buffer{create_buffer(device_,
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    mapped_memory staging_map{
        map_memory(device_, staging_buffer.memory, image_size)};
    memcpy(staging_map.mapped_memory, pixels, static_cast<size_t>(image_size));
    unmap_memory(device_, &staging_map);
    stbi_image_free(pixels);

    VkExtent2D const extent{static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};
    vulkan_image texture{create_image_and_view(device_,
        extent,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT)};

    vulkan_queue* const queue{device_->present_queue};

    VkCommandBuffer present_queue_buffer; // NOLINT
    begin_single_time_commands(device_,
        queue->command_pool,
        1,
        std::span{&present_queue_buffer, 1});

    transition_image(texture.image,
        present_queue_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(present_queue_buffer,
        staging_buffer.buffer,
        texture.image,
        extent);
    transition_image(texture.image,
        present_queue_buffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    end_single_time_commands(device_,
        queue->queue,
        std::span{&present_queue_buffer, 1},
        queue->command_pool);

    destroy(device_, &staging_buffer);

    return texture;
}

vkrndr::vulkan_font vkrndr::vulkan_renderer::load_font(
    std::filesystem::path const& font_path,
    uint32_t const font_size)
{
    font_bitmap font_bitmap{
        font_manager_->load_font_bitmap(font_path, font_size)};

    auto const image_size{static_cast<VkDeviceSize>(
        font_bitmap.bitmap_width * font_bitmap.bitmap_height)};

    vulkan_buffer staging_buffer{create_buffer(device_,
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    mapped_memory staging_map{
        map_memory(device_, staging_buffer.memory, image_size)};
    memcpy(staging_map.mapped_memory,
        font_bitmap.bitmap_data.data(),
        static_cast<size_t>(image_size));
    unmap_memory(device_, &staging_map);

    VkExtent2D const extent{font_bitmap.bitmap_width,
        font_bitmap.bitmap_height};
    vulkan_image const texture{create_image_and_view(device_,
        extent,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT)};

    vulkan_queue* const queue{device_->present_queue};

    VkCommandBuffer present_queue_buffer; // NOLINT
    begin_single_time_commands(device_,
        queue->command_pool,
        1,
        std::span{&present_queue_buffer, 1});

    transition_image(texture.image,
        present_queue_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(present_queue_buffer,
        staging_buffer.buffer,
        texture.image,
        extent);
    transition_image(texture.image,
        present_queue_buffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    end_single_time_commands(device_,
        queue->queue,
        std::span{&present_queue_buffer, 1},
        queue->command_pool);

    destroy(device_, &staging_buffer);

    return {std::move(font_bitmap.bitmaps),
        font_bitmap.bitmap_width,
        font_bitmap.bitmap_height,
        texture};
}

void vkrndr::vulkan_renderer::record_command_buffer(vulkan_scene* scene,
    VkCommandBuffer& command_buffer)
{
    vkResetCommandBuffer(command_buffer, 0);

    VkFormat const format{swap_chain_->image_format()};
    VkCommandBufferInheritanceRenderingInfo rendering_inheritance_info{};
    rendering_inheritance_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
    rendering_inheritance_info.colorAttachmentCount = 1;
    rendering_inheritance_info.pColorAttachmentFormats = &format;
    rendering_inheritance_info.rasterizationSamples = device_->max_msaa_samples;

    VkCommandBufferInheritanceInfo inheritance_info{};
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = &rendering_inheritance_info;

    VkCommandBufferBeginInfo secondary_begin_info{};
    secondary_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    secondary_begin_info.flags =
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    secondary_begin_info.pInheritanceInfo = &inheritance_info;
    check_result(vkBeginCommandBuffer(command_buffer, &secondary_begin_info));

    scene->draw(command_buffer, swap_chain_->extent());

    if (imgui_layer_)
    {
        scene->draw_imgui();
        imgui_layer_->draw(command_buffer);
    }

    check_result(vkEndCommandBuffer(command_buffer));
}

bool vkrndr::vulkan_renderer::is_multisampled() const
{
    return device_->max_msaa_samples != VK_SAMPLE_COUNT_1_BIT;
}

void vkrndr::vulkan_renderer::recreate()
{
    if (is_multisampled())
    {
        cleanup_images();
        color_image_ = create_image_and_view(device_,
            swap_chain_->extent(),
            1,
            device_->max_msaa_samples,
            swap_chain_->image_format(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void vkrndr::vulkan_renderer::cleanup_images()
{
    destroy(device_, &color_image_);
}
