#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace regimeflow {

class Sha256 {
public:
    Sha256();

    void update(const void* data, size_t len);
    std::array<uint8_t, 32> digest();

private:
    void transform(const uint8_t* block);

    uint64_t bit_len_ = 0;
    uint32_t state_[8];
    uint8_t buffer_[64];
    size_t buffer_len_ = 0;
};

}  // namespace regimeflow
