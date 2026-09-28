#ifndef BYTEORDER_STUFFS_HPP
#define BYTEORDER_STUFFS_HPP

#include <bit>
#include <cstdint>

// Converts an integer to Big-Endian
template <typename T>
T to_big_endian(T value) {
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value); // Swaps bytes if system is Little-Endian
    }
    return value; // Already Big-Endian
}

// Reverts a Big-Endian value back to native system endianness
template <typename T>
T from_big_endian(T value) {
    return to_big_endian(value); // Swapping bytes again restores original order
}


#endif