#include <uci_parser.hpp>

#include <catch2/catch_test_macros.hpp>

#include <boost/optional/optional_io.hpp>
#include <boost/spirit/home/x3.hpp>

#include <fmt/core.h>

#include <cstdint>
#include <iostream>

template<>
struct Catch::StringMaker<boost::optional<std::pair<int64_t, int64_t>>>
{
    static std::string convert(
        boost::optional<std::pair<int64_t, int64_t>> const& value)
    {
        if (value)
        {
            return fmt::format("{} {}", value->first, value->second);
        }
        return "nullopt";
    }
};

TEST_CASE("id", "[uci]")
{
    using namespace std::string_view_literals;

    using boost::spirit::x3::ascii::space;

    // id name Stockfish 16.1
    // id author the Stockfish developers (see AUTHORS file)
    SECTION("name")
    {
        auto const string{"id name Stockfish 16.1"sv};
        auto iter{string.cbegin()};

        pawn::ast::id id;
        CHECK(phrase_parse(iter, string.cend(), pawn::id(), space, id));
        CHECK(iter == string.cend());
        CHECK(id.key == "name");
        CHECK(id.value == "Stockfish 16.1");
    }

    SECTION("author")
    {
        auto const string{
            "id author the Stockfish developers (see AUTHORS file)"sv};
        auto iter{string.cbegin()};

        pawn::ast::id id;
        CHECK(phrase_parse(iter, string.cend(), pawn::id(), space, id));
        CHECK(iter == string.cend());
        CHECK(id.key == "author");
        CHECK(id.value == "the Stockfish developers (see AUTHORS file)");
    }
}

TEST_CASE("option", "[uci]")
{
    using namespace std::string_view_literals;

    using boost::spirit::x3::ascii::space;

    SECTION("check")
    {
        auto const string{"option name Nullmove type check default true\n"sv};
        auto iter{string.cbegin()};

        pawn::ast::option option;
        CHECK(phrase_parse(iter, string.cend(), pawn::option(), space, option));
        CHECK(option.name == "Nullmove");
        CHECK(option.type == pawn::ast::option_type::check);
        CHECK_FALSE(option.min_max);
        CHECK(option.def.get_value_or("") == "true");
        CHECK(option.values.empty());
    }

    SECTION("spin")
    {
        auto const string{
            "option name Selectivity type spin default 2 min 0 max 4\n"sv};
        auto iter{string.cbegin()};

        pawn::ast::option option;
        CHECK(phrase_parse(iter, string.cend(), pawn::option(), space, option));
        CHECK(option.name == "Selectivity");
        CHECK(option.type == pawn::ast::option_type::spin);

        auto const min_max{
            option.min_max.get_value_or(std::pair<int64_t, int64_t>{-1, -1})};
        CHECK(min_max.first == 0);
        CHECK(min_max.second == 4);
        CHECK(option.def.get_value_or("") == "2");
        CHECK(option.values.empty());
    }

    SECTION("combo")
    {
        using namespace std::string_literals;

        auto const string{
            "option name Style type combo default Normal var Solid var Normal var Risky\n"sv};
        auto iter{string.cbegin()};

        pawn::ast::option option;
        CHECK(phrase_parse(iter, string.cend(), pawn::option(), space, option));
        CHECK(option.name == "Style");
        CHECK(option.type == pawn::ast::option_type::combo);
        CHECK_FALSE(option.min_max);
        CHECK(option.def.get_value_or("") == "Normal");
        CHECK(option.values == std::vector{"Solid"s, "Normal"s, "Risky"s});
    }

    SECTION("string")
    {
        auto const string{
            "option name NalimovPath type string default c:\\n"sv};
        auto iter{string.cbegin()};

        pawn::ast::option option;
        CHECK(phrase_parse(iter, string.cend(), pawn::option(), space, option));
        CHECK(option.name == "NalimovPath");
        CHECK(option.type == pawn::ast::option_type::string);
        CHECK_FALSE(option.min_max);
        CHECK(option.def.get_value_or("") == "c:\\n");
        CHECK(option.values.empty());
    }

    SECTION("button")
    {
        auto const string{"option name Clear Hash type button\n"sv};
        auto iter{string.cbegin()};

        pawn::ast::option option;
        CHECK(phrase_parse(iter, string.cend(), pawn::option(), space, option));
        CHECK(option.name == "Clear Hash");
        CHECK(option.type == pawn::ast::option_type::button);
        CHECK_FALSE(option.min_max);
        CHECK_FALSE(option.def);
        CHECK(option.values.empty());
    }
}

TEST_CASE("uciok", "[uci]")
{
    using namespace std::string_view_literals;

    using boost::spirit::x3::ascii::space;

    auto const string{"uciok"sv};
    auto iter{string.cbegin()};

    pawn::ast::uciok uciok;
    CHECK(phrase_parse(iter, string.cend(), pawn::uciok(), space, uciok));
    CHECK(iter == string.cend());
}
