/**
 * @file sha256.h
 * @brief RegimeFlow regimeflow sha256 declarations.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace regimeflow {

/**
 * @brief Streaming SHA-256 hash implementation.
 */
class Sha256 {
public:
    /**
     * @brief Construct a new SHA-256 hasher with initial state.
     */
    Sha256();

    /**
     * @brief Add bytes to the hash.
     * @param data Pointer to data.
     * @param len Length in bytes.
     */
    void update(const void* data, size_t len);
    /**
     * @brief Finalize and return the hash digest.
     * @return 32-byte SHA-256 digest.
     */
    std::array<uint8_t, 32> digest();

private:
    void transform(const uint8_t* block);

    uint64_t bit_len_ = 0;
    uint32_t state_[8];
    uint8_t buffer_[64];
    size_t buffer_len_ = 0;
};

}  // namespace regimeflow
