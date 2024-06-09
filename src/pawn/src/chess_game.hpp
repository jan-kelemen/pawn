#ifndef PAWN_CHESS_GAME_INCLUDED
#define PAWN_CHESS_GAME_INCLUDED

#include <chess.hpp>
#include <scene.hpp>
#include <uci_engine.hpp>

#include <array>
#include <future>
#include <string>
#include <string_view>
#include <vector>

namespace vkrndr
{
    struct vulkan_device;
    class vulkan_renderer;
    class vulkan_scene;
} // namespace vkrndr

namespace pawn
{
    struct [[nodiscard]] board_piece final
    {
        piece_color color{piece_color::none};
        piece_type type{piece_type::none};
        bool moved_from_starting_position{false};
    };

    struct [[nodiscard]] board_state final
    {
        std::array<board_piece, 64> tiles;
    };

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

        void update();

        void end_frame();

    public:
        chess_game& operator=(chess_game const&) = delete;

        chess_game& operator=(chess_game&&) noexcept = delete;

    private:
        uci_engine engine_;
        orthographic_camera camera_;
        scene scene_;
        board_state board_;
        std::vector<std::string> moves_;
        std::future<std::string> next_move_{};
    };
} // namespace pawn

#endif
