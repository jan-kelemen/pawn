#include <chess_game.hpp>

#include <scene.hpp>
#include <uci_engine.hpp>

#include <cppext_numeric.hpp>

#include <cstdint>
#include <initializer_list>
#include <ranges>

pawn::chess_game::chess_game(std::string_view engine_command_line)
    : engine_{engine_command_line}
    , scene_{engine_}
{
}

void pawn::chess_game::attach_renderer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_renderer* renderer)
{
    scene_.attach_renderer(device, renderer);
}

void pawn::chess_game::detach_renderer() { scene_.detach_renderer(); }

void pawn::chess_game::begin_frame()
{
    scene_.begin_frame();

    std::initializer_list<pawn::piece_type> home_row{pawn::piece_type::rook,
        pawn::piece_type::knight,
        pawn::piece_type::bishop,
        pawn::piece_type::queen,
        pawn::piece_type::king,
        pawn::piece_type::bishop,
        pawn::piece_type::knight,
        pawn::piece_type::rook};

    for (auto const& [index, piece] : std::views::enumerate(home_row))
    {
        scene_.add_piece(pawn::to_board_peice(0,
            cppext::narrow<uint8_t>(index),
            pawn::mesh_color::white,
            piece,
            piece == pawn::piece_type::queen));
        scene_.add_piece(pawn::to_board_peice(1,
            cppext::narrow<uint8_t>(index),
            pawn::mesh_color::white,
            pawn::piece_type::pawn,
            false));
        scene_.add_piece(pawn::to_board_peice(6,
            cppext::narrow<uint8_t>(index),
            pawn::mesh_color::black,
            pawn::piece_type::pawn,
            false));
        scene_.add_piece(pawn::to_board_peice(7,
            cppext::narrow<uint8_t>(index),
            pawn::mesh_color::black,
            piece,
            piece == pawn::piece_type::king));
    }

    scene_.update(camera_);
}

void pawn::chess_game::end_frame() { scene_.end_frame(); }
