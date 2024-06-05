#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/optional/optional.hpp>
#include <boost/spirit/home/x3.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace pawn::ast
{
    struct [[nodiscard]] id final
    {
        std::string key;
        std::string value;
    };

    enum option_type
    {
        check,
        spin,
        combo,
        button,
        string
    };

    struct [[nodiscard]] option final
    {
        std::string name;
        option_type type;
        boost::optional<std::string> def;
        boost::optional<std::pair<int64_t, int64_t>> min_max;
        std::vector<std::string> values;
    };

    struct [[nodiscard]] uciok final
    {
        bool dummy;
    };
} // namespace pawn::ast

namespace pawn
{
    namespace parser
    {
        namespace x3 = boost::spirit::x3;

        using id_type = x3::rule<class id, ast::id>;
        BOOST_SPIRIT_DECLARE(id_type);

        using option_type = x3::rule<class option, ast::option>;
        BOOST_SPIRIT_DECLARE(option_type);

        using uciok_type = x3::rule<class uciok, ast::uciok>;
        BOOST_SPIRIT_DECLARE(uciok_type);
    } // namespace parser

    parser::id_type id();

    parser::option_type option();

    parser::uciok_type uciok();
} // namespace pawn
