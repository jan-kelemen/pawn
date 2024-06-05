#include <uci_engine.hpp>

#define BOOST_PROCESS_USE_STD_FS
#include <boost/process.hpp>

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
        input_ << "uci" << std::endl;

        std::string line;

        while (child_.running() && std::getline(output_, line) && !line.empty())
        {
            spdlog::info(line);
            if (line.starts_with("uciok"))
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

private:
    bp::opstream input_;
    bp::ipstream output_;
    bp::child child_;
};

pawn::uci_engine::uci_engine(std::string_view command_line)
    : impl_{std::make_unique<impl>(command_line)}
{
}

pawn::uci_engine::uci_engine(uci_engine&&) noexcept = default;

pawn::uci_engine::~uci_engine() = default;
