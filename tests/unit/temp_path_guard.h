#pragma once

#include <filesystem>
#include <system_error>
#include <utility>

namespace regimeflow::test
{
    class TempPathGuard
    {
    public:
        explicit TempPathGuard(std::filesystem::path path)
            : path_(std::move(path))
        {
            cleanup();
        }

        TempPathGuard(const TempPathGuard&) = delete;
        TempPathGuard& operator=(const TempPathGuard&) = delete;

        TempPathGuard(TempPathGuard&&) = delete;
        TempPathGuard& operator=(TempPathGuard&&) = delete;

        ~TempPathGuard() { cleanup(); }

        [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    private:
        void cleanup() const noexcept
        {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
        }

        std::filesystem::path path_;
    };
}  // namespace regimeflow::test
