#include "regimeflow/data/mmap_writer.h"

#include "regimeflow/common/sha256.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace regimeflow::data {

namespace {

constexpr char kMagic[8] = {'R', 'G', 'M', 'F', 'L', 'O', 'W', '1'};
constexpr uint32_t kFileVersion = 1;

bool is_finite(double value) {
    return std::isfinite(value);
}

void write_bytes(std::ofstream& out, const void* data, size_t len, Sha256* sha) {
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
    if (sha) {
        sha->update(data, len);
    }
}

int32_t yyyymmdd_from_timestamp(const Timestamp& ts) {
    auto text = ts.to_string("%Y%m%d");
    return static_cast<int32_t>(std::stoi(text));
}

}  // namespace

Result<void> MmapWriter::write_bars(const std::string& path,
                                    const std::string& symbol,
                                    BarType bar_type,
                                    std::vector<Bar> bars) {
    std::sort(bars.begin(), bars.end(), [](const Bar& a, const Bar& b) {
        return a.timestamp < b.timestamp;
    });
    if (auto validation = validate_bars(bars); validation.is_err()) {
        return validation;
    }

    const size_t count = bars.size();
    std::vector<int64_t> timestamps(count);
    std::vector<double> opens(count);
    std::vector<double> highs(count);
    std::vector<double> lows(count);
    std::vector<double> closes(count);
    std::vector<uint64_t> volumes(count);

    for (size_t i = 0; i < count; ++i) {
        timestamps[i] = bars[i].timestamp.microseconds();
        opens[i] = bars[i].open;
        highs[i] = bars[i].high;
        lows[i] = bars[i].low;
        closes[i] = bars[i].close;
        volumes[i] = bars[i].volume;
    }

    std::vector<DateIndex> index = build_date_index(bars);

    FileHeader header{};
    std::memcpy(header.magic, kMagic, sizeof(kMagic));
    header.version = kFileVersion;
    header.flags = 0;
    std::memset(header.symbol, 0, sizeof(header.symbol));
    std::strncpy(header.symbol, symbol.c_str(), sizeof(header.symbol) - 1);
    header.bar_type = static_cast<uint32_t>(bar_type);
    header.bar_size_ms = bar_size_ms(bar_type);
    if (!bars.empty()) {
        header.start_timestamp = bars.front().timestamp.microseconds();
        header.end_timestamp = bars.back().timestamp.microseconds();
    }
    header.bar_count = static_cast<uint64_t>(count);
    header.data_offset = sizeof(FileHeader);

    size_t data_bytes = count * (sizeof(int64_t) + 4 * sizeof(double) + sizeof(uint64_t));
    header.index_offset = header.data_offset + static_cast<uint64_t>(data_bytes);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return Error(Error::Code::IoError, "Unable to open mmap output file");
    }

    write_bytes(out, &header, sizeof(header), nullptr);

    Sha256 sha;
    write_bytes(out, timestamps.data(), timestamps.size() * sizeof(int64_t), &sha);
    write_bytes(out, opens.data(), opens.size() * sizeof(double), &sha);
    write_bytes(out, highs.data(), highs.size() * sizeof(double), &sha);
    write_bytes(out, lows.data(), lows.size() * sizeof(double), &sha);
    write_bytes(out, closes.data(), closes.size() * sizeof(double), &sha);
    write_bytes(out, volumes.data(), volumes.size() * sizeof(uint64_t), &sha);

    if (!index.empty()) {
        write_bytes(out, index.data(), index.size() * sizeof(DateIndex), nullptr);
    }

    auto checksum = sha.digest();
    std::memcpy(header.checksum, checksum.data(), checksum.size());

    out.seekp(0);
    write_bytes(out, &header, sizeof(header), nullptr);
    return Ok();
}

Result<void> MmapWriter::validate_bars(const std::vector<Bar>& bars) const {
    if (bars.empty()) {
        return Ok();
    }
    Timestamp last = bars.front().timestamp;
    for (size_t i = 0; i < bars.size(); ++i) {
        const auto& bar = bars[i];
        if (bar.timestamp.microseconds() <= 0) {
            return Error(Error::Code::InvalidArgument, "Bar timestamp must be positive");
        }
        if (i > 0 && bar.timestamp < last) {
            return Error(Error::Code::InvalidArgument, "Bars must be sorted by timestamp");
        }
        if (!is_finite(bar.open) || !is_finite(bar.high) || !is_finite(bar.low) ||
            !is_finite(bar.close)) {
            return Error(Error::Code::InvalidArgument, "Bar prices must be finite");
        }
        if (bar.open <= 0 || bar.high <= 0 || bar.low <= 0 || bar.close <= 0) {
            return Error(Error::Code::InvalidArgument, "Bar prices must be positive");
        }
        if (bar.high < bar.low) {
            return Error(Error::Code::InvalidArgument, "Bar high must be >= low");
        }
        last = bar.timestamp;
    }
    return Ok();
}

uint32_t MmapWriter::bar_size_ms(BarType type) {
    switch (type) {
        case BarType::Time_1Min: return 60'000;
        case BarType::Time_5Min: return 5 * 60'000;
        case BarType::Time_15Min: return 15 * 60'000;
        case BarType::Time_30Min: return 30 * 60'000;
        case BarType::Time_1Hour: return 60 * 60'000;
        case BarType::Time_4Hour: return 4 * 60 * 60'000;
        case BarType::Time_1Day: return 24 * 60 * 60'000;
        case BarType::Volume: return 0;
        case BarType::Tick: return 0;
        case BarType::Dollar: return 0;
    }
    return 0;
}

std::vector<DateIndex> MmapWriter::build_date_index(const std::vector<Bar>& bars) {
    std::vector<DateIndex> index;
    int32_t last_date = 0;
    for (size_t i = 0; i < bars.size(); ++i) {
        int32_t date = yyyymmdd_from_timestamp(bars[i].timestamp);
        if (i == 0 || date != last_date) {
            index.push_back(DateIndex{date, static_cast<uint64_t>(i)});
            last_date = date;
        }
    }
    return index;
}

}  // namespace regimeflow::data
