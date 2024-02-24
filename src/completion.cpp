//
//  completion.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 8/30/23.
//

#include "completion.hpp"
#include "log.hpp"

#include <clang-c/Index.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

constexpr int excludeDeclsFromPCH = 0;
constexpr int displayDiagnostics = 0;
//constexpr unsigned priorityThreshold = 25;

auto logger()
{
    return spdlog::get(ocls::LogName::completion);
}

std::unordered_map<CXCompletionChunkKind, std::string> completeChunkKindSpellingMap = {
{CXCompletionChunk_Optional,            "Optional"},
    {CXCompletionChunk_TypedText,       "TypedText"},
    {CXCompletionChunk_Text,            "Text"},
    {CXCompletionChunk_Placeholder,     "Placeholder"},
    {CXCompletionChunk_Informative,     "Informative"},
    {CXCompletionChunk_CurrentParameter,"CurrentParameter"},
    {CXCompletionChunk_LeftParen,       "LeftParen"},
    {CXCompletionChunk_RightParen,      "RightParen"},
    {CXCompletionChunk_LeftBracket,     "LeftBracket"},
    {CXCompletionChunk_RightBracket,    "RightBracket"},
    {CXCompletionChunk_LeftBrace,       "LeftBrace"},
    {CXCompletionChunk_RightBrace,      "RightBrace"},
    {CXCompletionChunk_LeftAngle,       "LeftAngle"},
    {CXCompletionChunk_RightAngle,      "RightAngle"},
    {CXCompletionChunk_Comma,           "Comma"},
    {CXCompletionChunk_ResultType,      "ResultType"},
    {CXCompletionChunk_Colon,           "Colon"},
    {CXCompletionChunk_SemiColon,       "SemiColon"},
    {CXCompletionChunk_Equal,           "Equal"},
    {CXCompletionChunk_HorizontalSpace, "HorizontalSpace"},
    {CXCompletionChunk_VerticalSpace,   "VerticalSpace"}
};

std::unordered_map<CXTokenKind, std::string> completeTokenKindSpellingMap = {
    {CXToken_Punctuation, "Punctuation"},
    {CXToken_Keyword, "Keyword"},
    {CXToken_Identifier, "Identifier"},
    {CXToken_Literal, "Literal"},
    {CXToken_Comment, "Comment"}
};

std::string getCompleteChunkKindSpelling(CXCompletionChunkKind chunkKind)
{
    auto valueIt = completeChunkKindSpellingMap.find(chunkKind);
    if(valueIt == completeChunkKindSpellingMap.end()) {
        return "Unknown";
    }
    return valueIt->second;
}

std::string getCompletionTokenKindSpelling(CXTokenKind availavility)
{
    auto valueIt = completeTokenKindSpellingMap.find(availavility);
    if(valueIt == completeTokenKindSpellingMap.end()) {
        return "Unknown";
    }
    return valueIt->second;
}

void showClangVersion()
{
    auto version = clang_getClangVersion();
    logger()->trace("{}", clang_getCString(version));
    clang_disposeString(version);
}

} // namespace

namespace ocls {

class Completion final : public ICompletion
{
public:
    Completion(std::vector<std::string> args)
        : m_args { std::move(args) }
    {
        m_cargs.reserve(m_args.size());
        for (auto& arg : m_args) {
            m_cargs.push_back(arg.c_str());
        }
    }

    std::vector<CompletionResult> GetCompletions(const std::string &fileName, unsigned lineno, unsigned columnno);

private:
    std::vector<std::string> m_args;
    std::vector<const char *> m_cargs;
};

std::vector<CompletionResult> Completion::GetCompletions(
    const std::string &fileName, unsigned lineno, unsigned columnno)
{
    std::vector<CompletionResult> completions;
    CXIndex index = clang_createIndex(excludeDeclsFromPCH, displayDiagnostics);
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(index, 
                                                                   fileName.c_str(),
                                                                   m_cargs.data(),
                                                                   static_cast<int>(m_cargs.size()),
                                                                   NULL,
                                                                   0,
                                                                   clang_defaultEditingTranslationUnitOptions());
    do
    {
        showClangVersion();

        if (!translationUnit)
        {
            logger()->error("Cannot parse translation unit");
            break;
        }

        CXFile file = clang_getFile(translationUnit, fileName.c_str());
        if (!file)
        {
            logger()->error("Unable to get file");
            break;
        }

        // Get the beginning of the file
        CXSourceLocation startLocation = clang_getLocationForOffset(translationUnit, file, 0);
        // Get the current cursor location
        CXSourceLocation loc = clang_getLocation(translationUnit, file, lineno, columnno);
        if (clang_equalLocations(loc, clang_getNullLocation()))
        {
            logger()->error("Invalid cursor location");
            break;
        }

        // Create a range from the start of the file to the cursor location
        CXToken *tokens = 0;
        unsigned int nTokens = 0;
        CXSourceRange range = clang_getRange(startLocation, loc);
        clang_tokenize(translationUnit, range, &tokens, &nTokens);
        if (nTokens <= 0)
        {
            logger()->error("No token at cursor");
            break;
        }

        // Get the start of the last token
        unsigned line, column;
        CXToken currentToken = tokens[nTokens - 1];
        CXSourceRange currentTokenRange = clang_getTokenExtent(translationUnit, currentToken);
        CXSourceLocation currentTokenStart = clang_getRangeStart(currentTokenRange);
        clang_getSpellingLocation(currentTokenStart, NULL, &line, &column, NULL);
        logger()->trace("The current token starts at line: {}, column: {}", line, column);

        // Get token
        CXString cxToken = clang_getTokenSpelling(translationUnit, currentToken);
        const char *cToken = clang_getCString(cxToken);
        bool shouldCompareToken = true;
        std::string token;
        if(cToken) {
            token = std::string(cToken);
            CXTokenKind kind = clang_getTokenKind(currentToken);
            auto tokenKind = getCompletionTokenKindSpelling(kind);
            logger()->trace("Token: {}", cToken);
            if(tokenKind == "Unknown" && strcmp(cToken, ".") == 0) {
                shouldCompareToken = false;
            }
        }
        clang_disposeString(cxToken);
        clang_disposeTokens(translationUnit, tokens, nTokens);

        // Code Completion
        CXCodeCompleteResults *compResults;
        compResults = clang_codeCompleteAt(translationUnit,
                                           fileName.c_str(),
                                           lineno,
                                           columnno,
                                           NULL,
                                           0,
                                           clang_defaultCodeCompleteOptions());
        if (!compResults) {
            logger()->trace("No completions available");
            break;
        }

        logger()->trace("Completions:", compResults->NumResults);
        for (auto i = 0U; i < compResults->NumResults; ++i) {
            const CXCompletionResult &result = compResults->Results[i];
            const CXCompletionString &compString = result.CompletionString;

            CXAvailabilityKind availability = clang_getCompletionAvailability(compString);
            if (CXAvailability_Available != availability) {
                continue;
            }

            const unsigned priority = clang_getCompletionPriority(compString);
//            if(priority > priorityThreshold) {
//                continue;
//            }

            const unsigned numChunks = clang_getNumCompletionChunks(compString);
            for (auto j = 0U; j < numChunks; j++) {
                CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(compString, j);
                if (chunkKind != CXCompletionChunk_TypedText) {
                    continue;
                }

                CXString chunkText = clang_getCompletionChunkText(compString, j);
                const char *completion = clang_getCString(chunkText);
                bool shouldContinue = false;
                if(!completion || strlen(completion) == 0) {
                    shouldContinue = true;
                }
                // Filter completions based on the token
                if (shouldCompareToken && strncmp(completion, token.c_str(), token.length()) != 0) {
                    shouldContinue = true;
                }
                clang_disposeString(chunkText);
                if(shouldContinue) {
                    continue;
                }

                // Print the rest of the completion info
                CXString kindName = clang_getCursorKindSpelling(result.CursorKind);
                logger()->trace("  Kind: {}", clang_getCString(kindName));
                clang_disposeString(kindName);
                logger()->trace("  Priority: {}", priority);
                logger()->trace("  Text: {}", getCompleteChunkKindSpelling(chunkKind), completion);
                completions.push_back(CompletionResult { completion, clang_getCString(kindName), ocls::CompletionItemKind::method, priority});
                // Stop after the first match.
                break;
            }
        }
        clang_disposeCodeCompleteResults(compResults);

    }
    while(false);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
    
    return completions;
}


std::shared_ptr<ICompletion> CreateCompletion(std::vector<std::string> args)
{
    return std::make_shared<Completion>(std::move(args));
}

} // namespace ocls
