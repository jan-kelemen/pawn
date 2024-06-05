#ifndef PAWN_UCI_ENGINE_INCLUDED
#define PAWN_UCI_ENGINE_INCLUDED

#include <memory>
#include <string_view>

namespace pawn
{
    class [[nodiscard]] uci_engine final
    {
    public:
        uci_engine(std::string_view command_line);

        uci_engine(uci_engine const&) = delete;

        uci_engine(uci_engine&&) noexcept;

    public:
        ~uci_engine();

    public:
        uci_engine& operator=(uci_engine const&) = delete;

        uci_engine& operator=(uci_engine&&) noexcept;

    private:
        class impl;
        std::unique_ptr<impl> impl_;
    };
} // namespace pawn

#endif
