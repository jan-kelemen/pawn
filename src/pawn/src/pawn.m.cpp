#include <sdl_window.hpp>
#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_scene.hpp>
#include <vulkan_swap_chain.hpp>

#include <imgui_impl_sdl2.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

#include <vulkan/vulkan_core.h>

#include <cstdlib>

// IWYU pragma: no_include <fmt/core.h>
// IWYU pragma: no_include <spdlog/common.h>

namespace
{
#ifdef NDEBUG
    constexpr bool enable_validation_layers{false};
#else
    constexpr bool enable_validation_layers{true};
#endif

    [[nodiscard]] bool is_quit_event(SDL_Event const& event,
        SDL_Window* const window)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            return true;
        case SDL_WINDOWEVENT:
        {
            SDL_WindowEvent const& window_event{event.window};
            return window_event.event == SDL_WINDOWEVENT_CLOSE &&
                window_event.windowID == SDL_GetWindowID(window);
        }
        default:
            return false;
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
    class [[nodiscard]] empty_scene : public vkrndr::vulkan_scene
    {
    public: // Destruction
        ~empty_scene() override = default;

    public: // Interface
        VkClearValue clear_color() override { return {{{1.f, .5f, .3f, 1.f}}}; }

        void draw([[maybe_unused]] VkCommandBuffer command_buffer,
            [[maybe_unused]] VkExtent2D extent) override
        {
        }

        void draw_imgui() override { }
    };
} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    vkrndr::sdl_guard const sdl_guard{SDL_INIT_VIDEO};

    vkrndr::sdl_window window{"pawn",
        static_cast<SDL_WindowFlags>(
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI),
        true,
        512,
        512};

    empty_scene scene;

    auto context{vkrndr::create_context(&window, enable_validation_layers)};
    auto device{vkrndr::create_device(context)};
    {
        vkrndr::vulkan_swap_chain swap_chain{&window, &context, &device};
        vkrndr::vulkan_renderer renderer{&window,
            &context,
            &device,
            &swap_chain};
        renderer.set_imgui_layer(enable_validation_layers);

        bool done{false};
        while (!done)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0)
            {
                if (enable_validation_layers)
                {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                }

                if (is_quit_event(event, window.native_handle()))
                {
                    done = true;
                }
            }

            renderer.begin_frame();

            renderer.draw(&scene);

            renderer.end_frame();
        }

        vkDeviceWaitIdle(device.logical);
    }
    destroy(&device);
    destroy(&context);
    return EXIT_SUCCESS;
}
