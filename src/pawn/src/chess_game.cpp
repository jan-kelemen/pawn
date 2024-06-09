#include <chess_game.hpp>

#include <scene.hpp>
#include <uci_engine.hpp>

#include <cppext_numeric.hpp>

#include <fmt/core.h>

#include <cstdint>
#include <initializer_list>
#include <ranges>
#include <span>

namespace
{
    [[nodiscard]] constexpr uint8_t to_flat_index(std::string_view string)
    {
        return (string[1] - '1') * 8 + (string[0] - 'a');
    }

    static_assert(to_flat_index("a1") == 0);
    static_assert(to_flat_index("e2") == 12);
    static_assert(to_flat_index("h8") == 63);

    void set_piece(pawn::board_state& state,
        uint8_t const row,
        uint8_t const column,
        pawn::board_piece const& piece)
    {
        state.tiles[row * 8 + column] = piece;
    }

    void set_to_starting_position(pawn::board_state& state)
    {
        for (auto& tile : state.tiles)
        {
            tile = pawn::board_piece{};
        }

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
            auto const column{cppext::narrow<uint8_t>(index)};

            set_piece(state,
                0,
                column,
                {.color = pawn::piece_color::white,
                    .type = piece,
                    .moved_from_starting_position = false});
            set_piece(state,
                1,
                column,
                {.color = pawn::piece_color::white,
                    .type = pawn::piece_type::pawn,
                    .moved_from_starting_position = false});
            set_piece(state,
                6,
                column,
                {.color = pawn::piece_color::black,
                    .type = pawn::piece_type::pawn,
                    .moved_from_starting_position = false});
            set_piece(state,
                7,
                column,
                {.color = pawn::piece_color::black,
                    .type = piece,
                    .moved_from_starting_position = false});
        }
    }
} // namespace

pawn::chess_game::chess_game(std::string_view engine_command_line)
    : engine_{engine_command_line}
    , scene_{engine_}
{
    set_to_starting_position(board_);
}

void pawn::chess_game::attach_renderer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_renderer* renderer)
{
    scene_.attach_renderer(device, renderer);
}

void pawn::chess_game::detach_renderer() { scene_.detach_renderer(); }

void pawn::chess_game::begin_frame() { scene_.begin_frame(); }

void pawn::chess_game::update()
{
    using namespace std::chrono_literals;
    if (!next_move_.valid())
    {
        next_move_ = std::async(
            [this]()
            {
                return engine_.next_move(fmt::format(
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR {} KQkq - 0 1",
                    moves_.size() % 2 ? 'b' : 'w'));
            });
    }
    else if (next_move_.wait_for(10ns) == std::future_status::ready)
    {
        std::string move{next_move_.get()};
        auto moved_piece{
            std::exchange(board_.tiles[to_flat_index(move.substr(0, 2))],
                board_piece{})};
        moved_piece.moved_from_starting_position = true;

        auto& new_tile{board_.tiles[to_flat_index(move.substr(2))]};
        new_tile = moved_piece;

        moves_.push_back(move);
    }

    for (auto const& [index, tile] : std::views::enumerate(board_.tiles))
    {
        if (tile.type == piece_type::none)
        {
            continue;
        }

        bool const highlighted{
            !moves_.empty() && to_flat_index(moves_.back().substr(2)) == index};
        scene_.add_piece(to_drawable_peice(static_cast<uint8_t>(index / 8),
            index % 8,
            tile.color,
            tile.type,
            highlighted));
    }
    scene_.update(camera_);
}

void pawn::chess_game::end_frame() { scene_.end_frame(); }
