/**
 * @file replay_journal.h
 * @brief Deterministic replay journal for engine events.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/events/event.h"

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace regimeflow::engine
{
    /**
     * @brief Serialize one engine event as a JSONL record.
     */
    [[nodiscard]] Result<std::string> serialize_replay_event(const events::Event& event);

    /**
     * @brief Parse one JSONL replay record into an engine event.
     */
    [[nodiscard]] Result<events::Event> parse_replay_event(std::string_view line);

    /**
     * @brief Read all replay events from a JSONL journal.
     */
    [[nodiscard]] Result<std::vector<events::Event>> read_replay_journal(const std::string& path);

    /**
     * @brief Append-only JSONL writer for deterministic replay/live parity traces.
     */
    class ReplayJournalWriter {
    public:
        explicit ReplayJournalWriter(std::string path);
        ~ReplayJournalWriter();

        ReplayJournalWriter(const ReplayJournalWriter&) = delete;
        ReplayJournalWriter& operator=(const ReplayJournalWriter&) = delete;
        ReplayJournalWriter(ReplayJournalWriter&&) noexcept = default;
        ReplayJournalWriter& operator=(ReplayJournalWriter&&) noexcept = default;

        [[nodiscard]] Result<void> append(const events::Event& event);
        [[nodiscard]] const std::string& path() const noexcept { return path_; }

    private:
        std::string path_;
        std::ofstream stream_;
    };
}  // namespace regimeflow::engine
