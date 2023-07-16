//
//  utils.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/21/21.
//

#include "utils.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>

namespace ocls::utils {

std::string GenerateId()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::string identifier;
    std::stringstream hex;
    for (auto i = 0; i < 16; ++i)
    {
        const auto rc = dis(gen);
        hex << std::hex << rc;
        auto str = hex.str();
        identifier.append(str.length() < 2 ? '0' + str : str);
        hex.str(std::string());
    }
    return identifier;
}

void Trim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

std::vector<std::string> SplitString(const std::string& str, const std::string& pattern)
{
    std::vector<std::string> result;
    const std::regex re(pattern);
    std::sregex_token_iterator iter(str.begin(), str.end(), re, -1);
    for (std::sregex_token_iterator end; iter != end; ++iter)
        result.push_back(iter->str());
    return result;
}

// Limited file uri -> path converter
std::string UriToPath(const std::string& uri)
{
    try
    {
        std::string str = uri;
        auto pos = str.find("file://");
        if (pos != std::string::npos)
            str.replace(pos, 7, "");
        do
        {
            pos = str.find("%3A");
            if (pos != std::string::npos)
                str.replace(pos, 3, ":");
        } while (pos != std::string::npos);

        do
        {
            pos = str.find("%20");
            if (pos != std::string::npos)
                str.replace(pos, 3, " ");
        } while (pos != std::string::npos);

#if defined(WIN32)
        // remove first /
        if (str.rfind("/", 0) == 0)
        {
            str.replace(0, 1, "");
        }
#endif
        return str;
    }
    catch (std::exception& e)
    {
        spdlog::error("Failed to convert file uri to path, {}", e.what());
    }
    return uri;
}

bool EndsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

namespace internal {
// Generates a lookup table for the checksums of all 8-bit values.
std::array<std::uint_fast32_t, 256> GenerateCRCLookupTable()
{
    auto const reversed_polynomial = std::uint_fast32_t {0xEDB88320uL};

    // This is a function object that calculates the checksum for a value,
    // then increments the value, starting from zero.
    struct byte_checksum
    {
        std::uint_fast32_t operator()() noexcept
        {
            auto checksum = static_cast<std::uint_fast32_t>(n++);

            for (auto i = 0; i < 8; ++i)
                checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);

            return checksum;
        }

        unsigned n = 0;
    };

    auto table = std::array<std::uint_fast32_t, 256> {};
    std::generate(table.begin(), table.end(), byte_checksum {});

    return table;
}
} // namespace internal

} // namespace ocls::utils
