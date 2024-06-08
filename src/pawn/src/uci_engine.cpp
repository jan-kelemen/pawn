#include <uci_engine.hpp>

#include <uci_parser.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/circular_buffer.hpp>
#define BOOST_PROCESS_USE_STD_FS
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <boost/spirit/home/x3.hpp>

#include <fmt/core.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace bp = boost::process;

class [[nodiscard]] pawn::uci_engine::impl final
{
public:
    explicit impl(std::string_view command_line)
        : child_{command_line.data(),
              bp::std_out > output_,
              bp::std_in < input_}
    {
        using boost::spirit::x3::ascii::space;

        send_command("uci");

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
            ast::uciok uciok; // NOLINT
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

    impl(impl const&) = delete;

    impl(impl&&) noexcept = delete;

public:
    ~impl()
    {
        using namespace std::chrono_literals;

        if (child_.running())
        {
            send_command("quit");
            std::this_thread::sleep_for(10ms);
            if (child_.running())
            {
                std::this_thread::sleep_for(500ms);
                child_.terminate();
            }
        }
    }

public:
    [[nodiscard]] std::string next_move(std::string_view fenstring)
    {
        using boost::spirit::x3::ascii::space;

        send_command(fmt::format("position fen {}", fenstring));
        send_command("go movetime 1000");

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
            ast::bestmove bestmove; // NOLINT
            if (phrase_parse(view.cbegin(),
                    view.cend(),
                    pawn::bestmove(),
                    space,
                    bestmove))
            {
                return std::move(bestmove.move);
            }
        }

        return "";
    }

    [[nodiscard]] std::span<std::string const> debug_output() const
    {
        auto data{debug_output_.array_one()};
        return {data.first, data.second};
    }

public:
    impl& operator=(impl const&) = delete;

    impl& operator=(impl&&) noexcept = delete;

private:
    void send_command(std::string_view command)
    {
        // NOLINTNEXTLINE(performance-avoid-endl)
        input_ << command << std::endl;
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

std::string pawn::uci_engine::next_move(std::string_view fenstring)
{
    return impl_->next_move(fenstring);
}

std::span<std::string const> pawn::uci_engine::debug_output() const
{
    return impl_->debug_output();
}
