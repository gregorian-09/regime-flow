#include "regimeflow/data/mmap_reader.h"
#include "regimeflow/data/mmap_writer.h"
#include "temp_path_guard.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

namespace regimeflow::data
{
namespace {

std::filesystem::path temp_mmap_path() {
    return std::filesystem::temp_directory_path() / "regimeflow_mmap_writer_checksum_test.rgmf";
}

}  // namespace

TEST(MmapWriter, PersistsComputedChecksumInHeader) {
    const auto path = temp_mmap_path();
    regimeflow::test::TempPathGuard temp_file(path);

    const auto symbol = SymbolRegistry::instance().intern("AAPL");
    std::vector<Bar> bars{
        Bar{Timestamp(1'000'000), symbol, 100.0, 102.0, 99.0, 101.0, 10'000},
        Bar{Timestamp(61'000'000), symbol, 101.0, 103.0, 100.0, 102.0, 12'000},
    };

    MmapWriter writer;
    const auto result = writer.write_bars(path.string(), "AAPL", BarType::Time_1Min, bars);
    ASSERT_TRUE(result.is_ok()) << result.error().to_string();

    FileHeader header{};
    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    input.read(reinterpret_cast<char*>(&header), sizeof(header));
    ASSERT_EQ(input.gcount(), static_cast<std::streamsize>(sizeof(header)));

    EXPECT_TRUE(std::ranges::any_of(header.checksum, [](const unsigned char byte) {
        return byte != 0;
    }));
}

}  // namespace regimeflow::data
