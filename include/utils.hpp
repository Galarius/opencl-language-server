//
//  utils.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/21/21.
//

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace ocls::utils {

struct IGenerator
{
    virtual ~IGenerator() = default;

    virtual std::string GenerateID() = 0;
};

struct IExitHandler
{
    virtual ~IExitHandler() = default;

    virtual void OnExit(int code) = 0;
};

std::shared_ptr<IGenerator> CreateDefaultGenerator();

std::shared_ptr<IExitHandler> CreateDefaultExitHandler();

std::string GetCurrentDateTime();

// --- String Helpers ---

std::vector<std::string> SplitString(const std::string& str, const std::string& pattern);

void Trim(std::string& s);

bool EndsWith(const std::string& str, const std::string& suffix);

inline std::string FormatBool(bool flag)
{
    return flag ? "yes" : "no";
}

// --- File Helpers ---

std::string UriToFilePath(const std::string& uri, bool isUnix);

std::string UriToFilePath(const std::string& uri);

std::optional<std::string> ReadFileContent(const std::string& fileName);

// --- CRC32 ---

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

} // namespace ocls::utils
