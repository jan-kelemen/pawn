#ifndef PAWN_CHESS_INCLUDED
#define PAWN_CHESS_INCLUDED

#include <cstdint>

namespace pawn
{
    enum class piece_type : uint8_t
    {
        none,
        rook,
        knight,
        bishop,
        queen,
        king,
        pawn
    };

    enum class piece_color : uint8_t
    {
        none,
        white,
        black
    };
} // namespace pawn

#endif // !PAWN_CHESS_INCLUDED
