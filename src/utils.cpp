//
//  utils.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/21/21.
//

#include "utils.hpp"

#include <algorithm>
#include <functional>
#include <fstream>
#include <iomanip>
#include <random>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>

#include <uriparser/Uri.h>

namespace ocls::utils {

// --- DefaultGenerator ---

class DefaultGenerator final : public IGenerator
{
public:
    DefaultGenerator() : gen(rd()), dis(0, 255) {}

    std::string GenerateID()
    {
        std::stringstream hex;
        hex << std::hex;
        for (auto i = 0; i < 16; ++i)
        {
            hex << std::setw(2) << std::setfill('0') << dis(gen);
        }
        return hex.str();
    }

private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dis;
};

std::shared_ptr<IGenerator> CreateDefaultGenerator()
{
    return std::make_shared<DefaultGenerator>();
}

// --- DefaultExitHandler ---

class DefaultExitHandler final : public IExitHandler
{
public:
    DefaultExitHandler() = default;

    void OnExit(int code)
    {
        exit(code);
    }
};

std::shared_ptr<IExitHandler> CreateDefaultExitHandler()
{
    return std::make_shared<DefaultExitHandler>();
}

// --- String Helpers ---

std::vector<std::string> SplitString(const std::string& str, const std::string& pattern)
{
    if (pattern.empty())
    {
        return {str};
    }
    std::vector<std::string> result;
    const std::regex re(pattern);
    std::sregex_token_iterator iter(str.begin(), str.end(), re, -1);
    for (std::sregex_token_iterator end; iter != end; ++iter)
    {
        result.push_back(iter->str());
    }
    return result;
}

void Trim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

bool EndsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

// --- File Helpers ---

std::string UriToFilePath(const std::string& uri, bool unix)
{
    const size_t bytesNeeded = 8 + 3 * uri.length() + 1;
    char* fileName = (char*)malloc(bytesNeeded * sizeof(char));
    if (unix)
    {
        if (uriUriStringToUnixFilenameA(uri.c_str(), fileName) != URI_SUCCESS)
        {
            free(fileName);
            throw std::runtime_error("Failed to convert URI to Unix filename.");
        }
    }
    else
    {
        if (uriUriStringToWindowsFilenameA(uri.c_str(), fileName) != URI_SUCCESS)
        {
            free(fileName);
            throw std::runtime_error("Failed to convert URI to Windows filename.");
        }
    }
    std::string result(fileName);
    free(fileName);
    return result;
}

std::string UriToFilePath(const std::string& uri)
{
#if defined(WIN32)
    return UriToFilePath(uri, false);
#else
    return UriToFilePath(uri, true);
#endif
}

std::optional<std::string> ReadFileContent(std::string_view fileName)
{
    std::string content;
    std::ifstream file(fileName);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        content = buffer.str();
    }
    else
    {
        spdlog::error("Unable to open file '{}'", fileName);
        return std::nullopt;
    }
    return content;
}

// --- CRC32 ---

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
            {
                checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);
            }
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
