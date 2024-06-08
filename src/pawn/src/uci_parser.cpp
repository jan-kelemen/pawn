#include <uci_parser.hpp>

#include <boost/fusion/adapted/std_pair.hpp> // IWYU pragma: keep
#include <boost/fusion/include/adapt_struct.hpp> // IWYU pragma: keep
#include <boost/fusion/include/std_pair.hpp> // IWYU pragma: keep

#include <string_view>

// IWYU pragma: no_include <boost/preprocessor.hpp>
// IWYU pragma: no_include <boost/mpl/aux_/preprocessor/is_seq.hpp>

BOOST_FUSION_ADAPT_STRUCT(pawn::ast::id, key, value)

BOOST_FUSION_ADAPT_STRUCT(pawn::ast::option, name, type, def, min_max, values)

BOOST_FUSION_ADAPT_STRUCT(pawn::ast::uciok, dummy)

BOOST_FUSION_ADAPT_STRUCT(pawn::ast::bestmove, move, ponder)

namespace pawn::parser
{
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;

    using iterator_type = std::string_view::const_iterator;
    using context_type = x3::phrase_parse_context<x3::ascii::space_type>::type;

    using x3::attr;
    using x3::alnum;
    using x3::graph;
    using x3::lit;
    using x3::int64;
    using x3::string;
    using x3::eol;
    using x3::lexeme;
    using ascii::char_;

    // id name Stockfish 16.1
    // id author the Stockfish developers (see AUTHORS file)
    // NOLINTNEXTLINE(readability-identifier-length)
    x3::rule<class id, ast::id> const id{"id"};

    auto const id_def = lit("id") >>
        (string("name") | string("author")) >> lexeme[+(char_ - eol)];

    BOOST_SPIRIT_DEFINE(id)

    BOOST_SPIRIT_INSTANTIATE(id_type, iterator_type, context_type)

    // option name Debug Log File type string default
    // option name Threads type spin default 1 min 1 max 1024
    // option name Hash type spin default 16 min 1 max 33554432
    // option name Clear Hash type button
    // option name Ponder type check default false
    // option name MultiPV type spin default 1 min 1 max 256
    // option name Skill Level type spin default 20 min 0 max 20
    // option name Move Overhead type spin default 10 min 0 max 5000
    // option name nodestime type spin default 0 min 0 max 10000
    // option name UCI_Chess960 type check default false
    // option name UCI_LimitStrength type check default false
    // option name UCI_Elo type spin default 1320 min 1320 max 3190
    // option name UCI_ShowWDL type check default false
    // option name SyzygyPath type string default <empty>
    // option name SyzygyProbeDepth type spin default 1 min 1 max 100
    // option name Syzygy50MoveRule type check default true
    // option name SyzygyProbeLimit type spin default 7 min 0 max 7
    // option name EvalFile type string default nn-b1a57edbea57.nnue
    // option name EvalFileSmall type string default nn-baff1ede1f90.nnue
    x3::rule<class option, ast::option> const option{"option"};

    struct option_types_ : x3::symbols<ast::option_type>
    {
        option_types_()
        {
            // clang-format off
            add
                ("check", ast::option_type::check)
                ("spin", ast::option_type::spin)
                ("combo", ast::option_type::combo)
                ("button", ast::option_type::button)
                ("string", ast::option_type::string)
            ;
            // clang-format on
        }
    } const option_types;

    auto const option_def = lit("option") >>
        lit("name") >> lexeme[+(char_ - lit(" type"))] >>
        lit("type") >> option_types >> -(lit("default") >> lexeme[+graph]) >>
        -(lit("min") >> int64 >> lit("max") >> int64) >>
        *(lit("var") >> lexeme[+graph]);

    BOOST_SPIRIT_DEFINE(option)

    BOOST_SPIRIT_INSTANTIATE(option_type, iterator_type, context_type)

    // uciok
    x3::rule<class uciok, ast::uciok> const uciok{"uciok"};

    auto const uciok_def = lit("uciok") >> attr(true);

    BOOST_SPIRIT_DEFINE(uciok)

    BOOST_SPIRIT_INSTANTIATE(uciok_type, iterator_type, context_type)

    // bestmove g1f3
    x3::rule<class bestmove, ast::bestmove> const bestmove{"bestmove"};

    auto const bestmove_def = lit("bestmove") >> lexeme[+alnum] >>
        -(lit("ponder") >> lexeme[+alnum]);

    BOOST_SPIRIT_DEFINE(bestmove)

    BOOST_SPIRIT_INSTANTIATE(bestmove_type, iterator_type, context_type)

} // namespace pawn::parser

pawn::parser::id_type pawn::id() { return parser::id; }

pawn::parser::option_type pawn::option() { return parser::option; }

pawn::parser::uciok_type pawn::uciok() { return parser::uciok; }

pawn::parser::bestmove_type pawn::bestmove() { return parser::bestmove; }
