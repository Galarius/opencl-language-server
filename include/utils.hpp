//
//  utils.hpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/21/21.
//

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace vscode::opencl::utils {

std::string GenerateId();
void Trim(std::string& s);
std::vector<std::string> SplitString(const std::string& str, const std::string& pattern);
std::string UriToPath(const std::string& uri);
bool EndsWith(const std::string& str, const std::string& suffix);
void RemoveNullTerminator(std::string& str);

namespace internal {
// Generates a lookup table for the checksums of all 8-bit values.
std::array<std::uint_fast32_t, 256> GenerateCRCLookupTable();
} // namespace internal

// https://rosettacode.org/wiki/CRC-32#C++
//
// Calculates the CRC for any sequence of values.
template <typename InputIterator>
std::uint_fast32_t CRC32(InputIterator first, InputIterator last)
{
    // Generate lookup table only on first use then cache it - this is thread-safe.
    static auto const table = internal::GenerateCRCLookupTable();

    // Calculate the checksum - make sure to clip to 32 bits, for systems that don't
    // have a true (fast) 32-bit type.
    return std::uint_fast32_t {0xFFFFFFFFuL} &
        ~std::accumulate(
               first,
               last,
               ~std::uint_fast32_t {0} & std::uint_fast32_t {0xFFFFFFFFuL},
               [](std::uint_fast32_t checksum, std::uint_fast8_t value) {
                   return table[(checksum ^ value) & 0xFFu] ^ (checksum >> 8);
               });
}

} // namespace vscode::opencl::utils
