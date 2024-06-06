#include <chess_game.hpp>

#include <sdl_window.hpp>
#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_swap_chain.hpp>

#include <cppext_numeric.hpp>

#include <imgui_impl_sdl2.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstdlib>
#include <ranges>

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

    pawn::chess_game game{argv[1]};

    auto context{vkrndr::create_context(&window, enable_validation_layers)};
    auto device{vkrndr::create_device(context)};
    {
        vkrndr::vulkan_swap_chain swap_chain{&window, &context, &device};
        vkrndr::vulkan_renderer renderer{&window,
            &context,
            &device,
            &swap_chain};
        renderer.set_imgui_layer(enable_validation_layers);

        game.attach_renderer(&device, &renderer);

        bool done{false};
        while (!done)
        {
            SDL_Event event;
            if (SDL_WaitEvent(&event) == 0)
            {
                continue;
            }

            if (enable_validation_layers)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
            }

            if (is_quit_event(event, window.native_handle()))
            {
                done = true;
            }

            renderer.begin_frame();
            game.begin_frame();

            renderer.draw(game.render_scene());

            game.end_frame();
            renderer.end_frame();
        }

        vkDeviceWaitIdle(device.logical);

        game.detach_renderer();
    }
    destroy(&device);
    destroy(&context);
    return EXIT_SUCCESS;
}
