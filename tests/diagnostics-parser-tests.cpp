//
//  diagnostics-parser-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 13/8/23.
//

#include "diagnostics.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>


using namespace ocls;
using namespace nlohmann;

class DiagnosticsParserRegexTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string, long, long, long, std::string>>
{};

TEST_P(DiagnosticsParserRegexTest, CheckRegexParsing)
{
    auto [input, expectedSource, expectedLine, expectedCol, expectedSeverity, expectedMessage] = GetParam();
    std::regex r("^(.*):(\\d+):(\\d+): ((fatal )?error|warning|Scholar): (.*)$");
    std::smatch match;
    EXPECT_TRUE(std::regex_search(input, match, r));

    auto parser = CreateDiagnosticsParser();
    auto [source, line, col, severity, message] = parser->ParseMatch(match);

    EXPECT_EQ(source, expectedSource);
    EXPECT_EQ(expectedLine, line);
    EXPECT_EQ(expectedCol, col);
    EXPECT_EQ(expectedSeverity, severity);
    EXPECT_EQ(expectedMessage, message);
}

INSTANTIATE_TEST_SUITE_P(
    DiagnosticsParserTest,
    DiagnosticsParserRegexTest,
    ::testing::Values(
        std::make_tuple(
            "<program source>:12:5: warning: no previous prototype for function 'getChannel'",
            "<program source>",
            11,
            5,
            2,
            "no previous prototype for function 'getChannel'"),
        std::make_tuple(
            "<program source>:16:27: error: use of undeclared identifier 'r'",
            "<program source>",
            15,
            27,
            1,
            "use of undeclared identifier 'r'"),
        std::make_tuple(
            "<custom source>:100:2: fatal error: unexpected end of file",
            "<custom source>",
            99,
            2,
            1,
            "unexpected end of file"),
        std::make_tuple(
            "<sample source>:5:14: Scholar: reference missing for citation",
            "<sample source>",
            4,
            14,
            -1,
            "reference missing for citation")));

TEST(ParseDiagnosticsTest, NoDiagnosticMessages)
{
    std::string log = "This is a regular log with no diagnostic message.";
    auto parser = CreateDiagnosticsParser();
    auto result = parser->ParseDiagnostics(log, "TestName", 10);
    EXPECT_EQ(result.size(), 0);
}

TEST(ParseDiagnosticsTest, SingleDiagnosticMessage)
{
    std::string log = "<program source>:12:5: warning: no previous prototype for function 'getChannel'";
    nlohmann::json expectedResult = R"([
        {
            "source": "TestName",
            "range": {
                "start": {
                    "line": 11,
                    "character": 5
                },
                "end": {
                    "line": 11,
                    "character": 5
                }
            },
            "severity": 2,
            "message": "no previous prototype for function 'getChannel'"
        }
    ])"_json;
    auto parser = CreateDiagnosticsParser();
    auto result = parser->ParseDiagnostics(log, "TestName", 10);
    EXPECT_EQ(result, expectedResult);
}

TEST(ParseDiagnosticsTest, MultipleDiagnosticMessages)
{
    std::string log = "<program source>:12:5: warning: no previous prototype for function 'getChannel'\n"
                      "<program source>:16:27: error: use of undeclared identifier 'r'";
    nlohmann::json expectedResult = R"([
        {
            "source": "TestName",
            "range": {
                "start": {
                    "line": 11,
                    "character": 5
                },
                "end": {
                    "line": 11,
                    "character": 5
                }
            },
            "severity": 2,
            "message": "no previous prototype for function 'getChannel'"
        },
        {
            "source": "TestName",
            "range": {
                "start": {
                    "line": 15,
                    "character": 27
                },
                "end": {
                    "line": 15,
                    "character": 27
                }
            },
            "severity": 1,
            "message": "use of undeclared identifier 'r'"
        }
    ])"_json;
    auto parser = CreateDiagnosticsParser();
    auto result = parser->ParseDiagnostics(log, "TestName", 10);
    EXPECT_EQ(result, expectedResult);
}

TEST(ParseDiagnosticsTest, ExceedProblemsLimit)
{
    std::string log = "<program source>:12:5: warning: no previous prototype for function 'getChannel'\n"
                      "<program source>:16:27: error: use of undeclared identifier 'r'\n"
                      "<program source>:25:7: warning: no previous prototype for function 'quadric'\n";
    nlohmann::json expectedResult = R"([
        {
            "source": "TestName",
            "range": {
                "start": {
                    "line": 11,
                    "character": 5
                },
                "end": {
                    "line": 11,
                    "character": 5
                }
            },
            "severity": 2,
            "message": "no previous prototype for function 'getChannel'"
        },
        {
            "source": "TestName",
            "range": {
                "start": {
                    "line": 15,
                    "character": 27
                },
                "end": {
                    "line": 15,
                    "character": 27
                }
            },
            "severity": 1,
            "message": "use of undeclared identifier 'r'"
        }
    ])"_json;
    auto parser = CreateDiagnosticsParser();
    auto result = parser->ParseDiagnostics(log, "TestName", 2);
    EXPECT_EQ(result, expectedResult);
}

TEST(ParseDiagnosticsTest, MalformedDiagnosticMessage)
{
    std::string log = "<sample source>:5:14: reference missing for citation";
    auto parser = CreateDiagnosticsParser();
    auto result = parser->ParseDiagnostics(log, "TestName", 10);
    EXPECT_EQ(result.size(), 0);
}
