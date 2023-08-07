//
//  utils.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/21/21.
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

namespace ocls::utils {

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

std::optional<std::string> ReadFileContent(std::string_view fileName)
{
    std::string content;
    std::ifstream file(fileName);
    if (file.is_open()) {
       std::stringstream buffer;
       buffer << file.rdbuf();
       file.close();
       content = buffer.str();
    } else {
        spdlog::error("Unable to open file '{}'", fileName);
        return std::nullopt;
    }
    return content;
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
