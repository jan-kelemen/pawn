#ifndef PAWN_CHESS_GAME_INCLUDED
#define PAWN_CHESS_GAME_INCLUDED

#include <scene.hpp>
#include <uci_engine.hpp>

#include <string_view>

namespace vkrndr
{
    struct vulkan_device;
    class vulkan_renderer;
    class vulkan_scene;
} // namespace vkrndr

namespace pawn
{
    class [[nodiscard]] chess_game final
    {
    public:
        explicit chess_game(std::string_view engine_command_line);

        chess_game(chess_game const&) = delete;

        chess_game(chess_game&&) noexcept = delete;

    public:
        ~chess_game() = default;

    public:
        [[nodiscard]] vkrndr::vulkan_scene* render_scene() { return &scene_; }

        void attach_renderer(vkrndr::vulkan_device* device,
            vkrndr::vulkan_renderer* renderer);

        void detach_renderer();

        void begin_frame();

        void end_frame();

    public:
        chess_game& operator=(chess_game const&) = delete;

        chess_game& operator=(chess_game&&) noexcept = delete;

    private:
        uci_engine engine_;
        orthographic_camera camera_;
        scene scene_;
    };
} // namespace pawn

#endif
