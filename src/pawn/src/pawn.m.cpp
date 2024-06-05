#include <scene.hpp>
#include <uci_engine.hpp>

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

    pawn::uci_engine engine(argv[1]);
    pawn::scene scene;

    auto context{vkrndr::create_context(&window, enable_validation_layers)};
    auto device{vkrndr::create_device(context)};
    {
        vkrndr::vulkan_swap_chain swap_chain{&window, &context, &device};
        vkrndr::vulkan_renderer renderer{&window,
            &context,
            &device,
            &swap_chain};
        renderer.set_imgui_layer(enable_validation_layers);

        scene.attach_renderer(&device, &renderer);

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
            scene.begin_frame();

            std::initializer_list<pawn::piece_type> home_row{
                pawn::piece_type::rook,
                pawn::piece_type::knight,
                pawn::piece_type::bishop,
                pawn::piece_type::queen,
                pawn::piece_type::king,
                pawn::piece_type::bishop,
                pawn::piece_type::knight,
                pawn::piece_type::rook};

            for (auto const& [index, piece] : std::views::enumerate(home_row))
            {
                scene.add_piece(pawn::to_board_peice(0,
                    cppext::narrow<uint8_t>(index),
                    pawn::mesh_color::white,
                    piece));
                scene.add_piece(pawn::to_board_peice(1,
                    cppext::narrow<uint8_t>(index),
                    pawn::mesh_color::white,
                    pawn::piece_type::pawn));
                scene.add_piece(pawn::to_board_peice(6,
                    cppext::narrow<uint8_t>(index),
                    pawn::mesh_color::black,
                    pawn::piece_type::pawn));
                scene.add_piece(pawn::to_board_peice(7,
                    cppext::narrow<uint8_t>(index),
                    pawn::mesh_color::black,
                    piece));
            }

            scene.update();
            renderer.draw(&scene);

            scene.end_frame();
            renderer.end_frame();
        }

        vkDeviceWaitIdle(device.logical);

        scene.detach_renderer();
    }
    destroy(&device);
    destroy(&context);
    return EXIT_SUCCESS;
}
