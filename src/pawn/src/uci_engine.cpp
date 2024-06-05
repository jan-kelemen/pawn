#include <uci_engine.hpp>

#include <uci_parser.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/circular_buffer.hpp>
#define BOOST_PROCESS_USE_STD_FS
#include <boost/process.hpp>
#include <boost/spirit/home/x3.hpp>

#include <spdlog/spdlog.h>

#include <iostream>
#include <thread>

namespace bp = boost::process;

class [[nodiscard]] pawn::uci_engine::impl final
{
public:
    impl(std::string_view command_line)
        : child_{command_line.data(),
              bp::std_out > output_,
              bp::std_in < input_}
    {
        using boost::spirit::x3::ascii::space;

        input_ << "uci" << std::endl;

        std::string line;

        while (std::getline(output_, line))
        {
            boost::algorithm::trim(line);
            if (line.empty())
            {
                continue;
            }
            debug_output_.push_back(line);

            std::string_view const view{line};
            ast::uciok uciok;
            if (phrase_parse(view.cbegin(),
                    view.cend(),
                    pawn::uciok(),
                    space,
                    uciok))
            {
                break;
            }
        }
    }

public:
    ~impl()
    {
        using namespace std::chrono_literals;

        if (child_.running())
        {
            input_ << "quit" << std::endl;
            std::this_thread::sleep_for(10ms);
            if (child_.running())
            {
                std::this_thread::sleep_for(500ms);
                child_.terminate();
            }
        }
    }

public:
    std::span<std::string const> debug_output() const
    {
        auto data{debug_output_.array_one()};
        return {data.first, data.second};
    }

private:
    bp::opstream input_;
    bp::ipstream output_;
    bp::child child_;

    boost::circular_buffer<std::string> debug_output_{50};
};

pawn::uci_engine::uci_engine(std::string_view command_line)
    : impl_{std::make_unique<impl>(command_line)}
{
}

pawn::uci_engine::uci_engine(uci_engine&&) noexcept = default;

pawn::uci_engine::~uci_engine() = default;

std::span<std::string const> pawn::uci_engine::debug_output() const
{
    return impl_->debug_output();
}
