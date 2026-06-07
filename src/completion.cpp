//
//  completion.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 8/30/23.
//

#include "completion.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <clang-c/Index.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <filesystem>

// Generated
#include "embedded_headers.h"

namespace fs = std::filesystem;

namespace {

constexpr int excludeDeclsFromPCH = 0;
constexpr int displayDiagnostics = 0;

auto logger()
{
    return spdlog::get(ocls::LogName::completion);
}

std::unordered_map<CXCompletionChunkKind, std::string> completeChunkKindSpellingMap = {
    {CXCompletionChunk_Optional, "Optional"},
    {CXCompletionChunk_TypedText, "TypedText"},
    {CXCompletionChunk_Text, "Text"},
    {CXCompletionChunk_Placeholder, "Placeholder"},
    {CXCompletionChunk_Informative, "Informative"},
    {CXCompletionChunk_CurrentParameter, "CurrentParameter"},
    {CXCompletionChunk_LeftParen, "LeftParen"},
    {CXCompletionChunk_RightParen, "RightParen"},
    {CXCompletionChunk_LeftBracket, "LeftBracket"},
    {CXCompletionChunk_RightBracket, "RightBracket"},
    {CXCompletionChunk_LeftBrace, "LeftBrace"},
    {CXCompletionChunk_RightBrace, "RightBrace"},
    {CXCompletionChunk_LeftAngle, "LeftAngle"},
    {CXCompletionChunk_RightAngle, "RightAngle"},
    {CXCompletionChunk_Comma, "Comma"},
    {CXCompletionChunk_ResultType, "ResultType"},
    {CXCompletionChunk_Colon, "Colon"},
    {CXCompletionChunk_SemiColon, "SemiColon"},
    {CXCompletionChunk_Equal, "Equal"},
    {CXCompletionChunk_HorizontalSpace, "HorizontalSpace"},
    {CXCompletionChunk_VerticalSpace, "VerticalSpace"}};

std::unordered_map<CXTokenKind, std::string> completeTokenKindSpellingMap = {
    {CXToken_Punctuation, "Punctuation"},
    {CXToken_Keyword, "Keyword"},
    {CXToken_Identifier, "Identifier"},
    {CXToken_Literal, "Literal"},
    {CXToken_Comment, "Comment"}};

std::unordered_map<CXErrorCode, std::string> translationErrorSpellingMap = {
    {CXError_Failure, "Failure"},
    {CXError_Crashed, "Crashed"},
    {CXError_InvalidArguments, "InvalidArguments"},
    {CXError_ASTReadError, "ASTReadError"},
    {CXError_Success, "Success"}};

struct TranslationUnitEntry
{
    CXTranslationUnit tu;
    CXIndex index;
};

std::string getCompleteChunkKindSpelling(CXCompletionChunkKind chunkKind)
{
    auto valueIt = completeChunkKindSpellingMap.find(chunkKind);
    if (valueIt == completeChunkKindSpellingMap.end())
    {
        return "Unknown";
    }
    return valueIt->second;
}

ocls::CompletionItemKind mapCursorKindToCompletionKind(CXCursorKind cursorKind)
{
    switch (cursorKind)
    {
        case CXCursor_FunctionDecl:
        case CXCursor_CXXMethod:
        case CXCursor_FunctionTemplate:
            return ocls::CompletionItemKind::function;

        case CXCursor_VarDecl:
        case CXCursor_ParmDecl:
            return ocls::CompletionItemKind::variable;

        case CXCursor_StructDecl:
            return ocls::CompletionItemKind::struct_;

        case CXCursor_UnionDecl:
            return ocls::CompletionItemKind::struct_;

        case CXCursor_ClassDecl:
            return ocls::CompletionItemKind::class_;

        case CXCursor_EnumDecl:
            return ocls::CompletionItemKind::enum_;

        case CXCursor_EnumConstantDecl:
            return ocls::CompletionItemKind::enumMember;

        case CXCursor_FieldDecl:
            return ocls::CompletionItemKind::field;

        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
            return ocls::CompletionItemKind::typeParameter;

        case CXCursor_MacroDefinition:
            return ocls::CompletionItemKind::snippet;

        case CXCursor_Constructor:
            return ocls::CompletionItemKind::constructor;

        case CXCursor_Namespace:
            return ocls::CompletionItemKind::module_;

        default:
            return ocls::CompletionItemKind::text;
    }
}

std::string getCompletionDetail(CXCompletionString compString)
{
    std::string detail;

    // Extract return type and parameters from completion chunks
    unsigned numChunks = clang_getNumCompletionChunks(compString);
    for (unsigned i = 0; i < numChunks; ++i)
    {
        CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(compString, i);
        CXString chunkText = clang_getCompletionChunkText(compString, i);
        const char *text = clang_getCString(chunkText);

        if (!text)
        {
            clang_disposeString(chunkText);
            continue;
        }

        switch (chunkKind)
        {
            case CXCompletionChunk_ResultType:
                detail += text;
                detail += " ";
                break;
            case CXCompletionChunk_LeftParen:
            case CXCompletionChunk_RightParen:
            case CXCompletionChunk_LeftBracket:
            case CXCompletionChunk_RightBracket:
            case CXCompletionChunk_Comma:
            case CXCompletionChunk_Colon:
            case CXCompletionChunk_SemiColon:
                detail += text;
                break;
            case CXCompletionChunk_Placeholder:
                detail += text;
                break;
            default:
                break;
        }
        clang_disposeString(chunkText);
    }

    return detail;
}

std::string getCompletionInsertText(CXCompletionString compString, bool &isSnippet)
{
    std::string insertText;
    isSnippet = false;
    int placeholderIndex = 1;

    unsigned numChunks = clang_getNumCompletionChunks(compString);
    for (unsigned i = 0; i < numChunks; ++i)
    {
        CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(compString, i);
        CXString chunkText = clang_getCompletionChunkText(compString, i);
        const char *text = clang_getCString(chunkText);

        if (!text)
        {
            clang_disposeString(chunkText);
            continue;
        }

        switch (chunkKind)
        {
            case CXCompletionChunk_TypedText:
                insertText += text;
                break;

            case CXCompletionChunk_Placeholder:
                // Turn placeholders into snippet tab stops: ${1:paramName}
                insertText += "${" + std::to_string(placeholderIndex++) + ":" + text + "}";
                isSnippet = true;
                break;

            case CXCompletionChunk_LeftParen:
            case CXCompletionChunk_RightParen:
            case CXCompletionChunk_LeftBracket:
            case CXCompletionChunk_RightBracket:
            case CXCompletionChunk_LeftBrace:
            case CXCompletionChunk_RightBrace:
            case CXCompletionChunk_LeftAngle:
            case CXCompletionChunk_RightAngle:
            case CXCompletionChunk_Comma:
            case CXCompletionChunk_Colon:
            case CXCompletionChunk_SemiColon:
            case CXCompletionChunk_HorizontalSpace:
                insertText += text;
                break;

            case CXCompletionChunk_ResultType:
            case CXCompletionChunk_Informative:
            case CXCompletionChunk_Optional:
            case CXCompletionChunk_VerticalSpace:
                // Skip — not part of the inserted text
                break;

            default:
                insertText += text;
                break;
        }

        clang_disposeString(chunkText);
    }

    // Add final cursor position $0 after the closing paren if it's a snippet
    if (isSnippet)
    {
        insertText += "$0";
    }

    return insertText;
}

std::string getBriefComment(CXCompletionString compString)
{
    std::string doc;
    CXString brief = clang_getCompletionBriefComment(compString);
    const char *cstr = clang_getCString(brief);
    if (cstr)
    {
        doc += cstr;
    }
    clang_disposeString(brief);
    return doc;
}

std::string getSortText(unsigned priority)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%05u", priority);
    return std::string(buffer);
}

std::vector<std::string> getCommitCharacters(ocls::CompletionItemKind kind)
{
    // Functions auto-commit with ( and ;
    if (kind == ocls::CompletionItemKind::function || kind == ocls::CompletionItemKind::constructor ||
        kind == ocls::CompletionItemKind::method)
    {
        return {"(", ";"};
    }
    // Other items auto-commit with ;
    return {";"};
}

std::vector<unsigned> getTags(bool deprecated)
{
    std::vector<unsigned> tags;
    if (deprecated)
    {
        tags.push_back(1); // CompletionItemTag::Deprecated
    }
    return tags;
}

bool shouldPreselect(unsigned priority)
{
    // Preselect very high priority items (priority <= 5)
    return priority <= 5;
}

} // namespace

namespace ocls {

class Completion final : public ICompletion
{
public:

    ~Completion() override
    {
        DestroyTranslationUnits();

        DeleteCache();
    }

    void OnFileOpen(const std::string &filePath, const std::string &content) override;
    void OnFileChange(const std::string &filePath, const std::string &content) override;
    void OnFileClose(const std::string &filePath) override;
    void SetTranslationOptions(const std::vector<std::string> &options) override;

    void SaveHeaders() override;
    std::vector<CompletionResult> GetCompletions(
        const std::string &filePath, unsigned lineno, unsigned columnno) override;

private:
    void DestroyTranslationUnits() noexcept;
    void DeleteCache() noexcept;

    std::vector<CXUnsavedFile> BuildUnsavedPool(const std::string &filePath, const std::string &content);

    std::vector<CompletionResult> FilterCompletions(CXCodeCompleteResults *compResults, const std::string &prefix);

    std::optional<std::string> GetPrefix(
        const CXTranslationUnit &translationUnit,
        const std::string &filePath,
        unsigned lineno,
        unsigned columnno,
        unsigned &startColumnno);

private:
    std::optional<fs::path> m_cacheDir;
    std::optional<fs::path> m_headersDir;
    std::vector<std::string> m_args;
    std::unordered_map<std::string, std::string> m_fileContents;
    std::unordered_map<std::string, TranslationUnitEntry> m_translationUnits;
};

void Completion::DestroyTranslationUnits() noexcept
{
    try
    {
        logger()->trace("Completion::DestroyTranslationUnits");
        for (auto &[filePath, entry] : m_translationUnits)
        {
            m_fileContents.erase(filePath);
            clang_disposeTranslationUnit(entry.tu);
            clang_disposeIndex(entry.index);
        }
    }
    catch (...)
    {}
}

void Completion::SaveHeaders()
{
#if defined(__APPLE__)
    auto temp = fs::temp_directory_path() / "com.galarius.opencl-language-server";
#else
    auto temp = fs::temp_directory_path() / "opencl-language-server";
#endif
    auto headersDir = temp / version / "headers";

    logger()->debug("Using cache dir: {}", temp.string());
    fs::create_directories(headersDir);
    // Unpack embedded headers onto the disk if they don't exist yet
    const auto &header_map = resources::get_headers();
    for (const auto &[filename, contents] : header_map)
    {
        fs::path targetFilePath = fs::path(headersDir) / filename;
        if (!fs::exists(targetFilePath))
        {
            std::ofstream file(targetFilePath, std::ios::binary);
            if (file.is_open())
            {
                file.write(contents.data(), contents.size());
            }
        }
    }

    m_cacheDir = temp;
    m_headersDir = headersDir;
}

void Completion::DeleteCache() noexcept
{
    try
    {
        if (m_cacheDir && fs::exists(*m_cacheDir))
        {
            auto cacheDir = m_cacheDir.value();
            logger()->debug("Deleting cache dir {}", cacheDir.string());
            fs::remove_all(cacheDir);
        }
    }
    catch (...)
    {}
}

std::vector<CXUnsavedFile> Completion::BuildUnsavedPool(const std::string &filePath, const std::string &content)
{
    std::vector<CXUnsavedFile> unsavedPool;
    unsavedPool.resize(1);
    unsavedPool[0].Filename = filePath.c_str();
    unsavedPool[0].Contents = content.data();
    unsavedPool[0].Length = static_cast<unsigned long>(content.size());
    return unsavedPool;
}

void Completion::OnFileOpen(const std::string &filePath, const std::string &content)
{
    logger()->trace("Completion::OnFileOpen - {}", filePath);
    if (!fs::exists(filePath))
    {
        logger()->error("File does not exist: {}", filePath);
        return;
    }

    m_fileContents[filePath] = content;

    CXIndex index = clang_createIndex(excludeDeclsFromPCH, displayDiagnostics);

    auto unsavedFiles = BuildUnsavedPool(filePath, content);
    unsigned options = clang_defaultEditingTranslationUnitOptions();
    options |= CXTranslationUnit_SkipFunctionBodies;
    options |= CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;

    std::vector<const char *> cargs;
    cargs.reserve(m_args.size());
    for (const auto &arg : m_args)
    {
        cargs.push_back(arg.c_str());
    }

    CXTranslationUnit tu;
    CXErrorCode code = clang_parseTranslationUnit2(
        index,
        filePath.c_str(),
        cargs.data(),
        static_cast<int>(cargs.size()),
        unsavedFiles.data(),
        static_cast<unsigned int>(unsavedFiles.size()),
        options,
        &tu);

    if (code != CXError_Success)
    {
        std::string error = translationErrorSpellingMap[code];
        logger()->error("Parsing failed with error code: {}", error);
        clang_disposeIndex(index);
        return;
    }

    m_translationUnits[filePath] = {tu, index};
}

void Completion::OnFileChange(const std::string &filePath, const std::string &content)
{
    logger()->trace("Completion::OnFileChange - {}", filePath);
    m_fileContents[filePath] = content;

    auto it = m_translationUnits.find(filePath);
    if (it == m_translationUnits.end())
    {
        logger()->debug("TU does not exist, performing full parse...");
        // No TU yet, do a full parse
        OnFileOpen(filePath, content);
        return;
    }

    auto unsavedFiles = BuildUnsavedPool(filePath, content);
    const unsigned options = clang_defaultReparseOptions(it->second.tu);

    // Reparse is much cheaper than a full parse —
    // reuses the precompiled preamble if it hasn't changed
    int result = clang_reparseTranslationUnit(
        it->second.tu, static_cast<unsigned int>(unsavedFiles.size()), unsavedFiles.data(), options);

    if (result != 0)
    {
        CXErrorCode code = static_cast<CXErrorCode>(result);
        std::string error = translationErrorSpellingMap[code];
        logger()->error("Failed to reparse TU: {}", error);
        clang_disposeTranslationUnit(it->second.tu);
        clang_disposeIndex(it->second.index);
        m_translationUnits.erase(it);
        OnFileOpen(filePath, content);
    }
}

void Completion::OnFileClose(const std::string &filePath)
{
    logger()->trace("Completion::OnFileClose - {}", filePath);
    m_fileContents.erase(filePath);

    auto it = m_translationUnits.find(filePath);
    if (it != m_translationUnits.end())
    {
        clang_disposeTranslationUnit(it->second.tu);
        clang_disposeIndex(it->second.index);
        m_translationUnits.erase(it);
    }
}

void Completion::SetTranslationOptions(const std::vector<std::string> &options)
{
    m_args.clear();

    for (const auto &arg : options)
    {
        m_args.push_back(arg);
    }

    if (m_headersDir)
    {
        m_args.push_back("-I" + m_headersDir.value().string());
        const auto &headers = resources::get_headers();
        for (const auto &[filename, _] : headers)
        {
            m_args.push_back("-include");
            m_args.push_back(filename);
        }
    }

    logger()->debug("Completion::SetTranslationOptions - {}", utils::FormatVector(m_args));

    // We need to re-parse sources with new options
    DestroyTranslationUnits();
}

std::optional<std::string> Completion::GetPrefix(
    const CXTranslationUnit &translationUnit,
    const std::string &filePath,
    unsigned lineno,
    unsigned columnno,
    unsigned &startColumnno)
{
    std::string prefix;
    CXFile file = clang_getFile(translationUnit, filePath.c_str());
    if (!file)
    {
        logger()->error("Unable to get file for {}", filePath);
        return std::nullopt;
    }

    CXSourceLocation startLoc = clang_getLocation(translationUnit, file, lineno, 1);
    CXSourceLocation endLoc = clang_getLocation(translationUnit, file, lineno, columnno);
    CXSourceRange safeRange = clang_getRange(startLoc, endLoc);
    CXToken *tokens = NULL;
    unsigned ntokens = 0;

    clang_tokenize(translationUnit, safeRange, &tokens, &ntokens);

    // Scan backwards from the end of the extracted tokens array
    bool found = false;
    for (int i = static_cast<int>(ntokens) - 1; i >= 0; --i)
    {
        CXSourceRange tokenRange = clang_getTokenExtent(translationUnit, tokens[i]);

        unsigned tLine, tStartCol, tEndCol;
        clang_getSpellingLocation(clang_getRangeStart(tokenRange), NULL, &tLine, &tStartCol, NULL);
        clang_getSpellingLocation(clang_getRangeEnd(tokenRange), NULL, NULL, &tEndCol, NULL);

        // - Must be on our target line
        // - The cursor column must intersect or directly touch this token
        if (tLine == lineno && columnno >= tStartCol && columnno <= tEndCol)
        {
            CXString spelling = clang_getTokenSpelling(translationUnit, tokens[i]);
            prefix = clang_getCString(spelling);
            // Save the start of the token
            startColumnno = tStartCol;
            clang_disposeString(spelling);
            found = true;
            break;
        }
    }

    if (tokens)
    {
        clang_disposeTokens(translationUnit, tokens, ntokens);
    }

    if (found)
    {
        logger()->debug("Target token: {}", prefix.c_str());
        return prefix;
    }
    else
    {
        logger()->warn("Cursor is sitting on a trailing newline or whitespace block.");
    }

    return std::nullopt;
}

std::vector<CompletionResult> Completion::FilterCompletions(
    CXCodeCompleteResults *compResults, const std::string &prefix)
{
    std::vector<CompletionResult> completions;
    for (auto i = 0U; i < compResults->NumResults; ++i)
    {
        const CXCompletionResult &result = compResults->Results[i];
        const CXCompletionString &compString = result.CompletionString;

        CXAvailabilityKind availability = clang_getCompletionAvailability(compString);
        // Skip items that are truly unavailable, but allow deprecated items
        if (availability == CXAvailability_NotAvailable || availability == CXAvailability_NotAccessible)
        {
            continue;
        }

        const bool isDeprecated = (availability == CXAvailability_Deprecated);
        const unsigned priority = clang_getCompletionPriority(compString);
        const unsigned numChunks = clang_getNumCompletionChunks(compString);
        for (auto j = 0U; j < numChunks; j++)
        {
            CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(compString, j);
            if (chunkKind != CXCompletionChunk_TypedText)
            {
                continue;
            }

            CXString chunkText = clang_getCompletionChunkText(compString, j);
            const char *completionCStr = clang_getCString(chunkText);

            if (!completionCStr || strlen(completionCStr) == 0)
            {
                clang_disposeString(chunkText);
                continue;
            }

            if (!prefix.empty() && strncmp(completionCStr, prefix.c_str(), prefix.length()) != 0)
            {
                clang_disposeString(chunkText);
                continue;
            }

            std::string label = completionCStr;
            clang_disposeString(chunkText);

            bool isSnippet;
            std::string detail = getCompletionDetail(compString);
            std::string insertText = getCompletionInsertText(compString, isSnippet);
            std::string documentation = getBriefComment(compString);
            CompletionItemKind itemKind = mapCursorKindToCompletionKind(result.CursorKind);
            std::string sortText = getSortText(priority);
            std::vector<std::string> commitChars = getCommitCharacters(itemKind);
            std::vector<unsigned> itemTags = getTags(isDeprecated);
            const bool preselect = shouldPreselect(priority);
            const unsigned resIndex = i;

            if (logger()->should_log(spdlog::level::trace))
            {
                CXString kindName = clang_getCursorKindSpelling(result.CursorKind);
                logger()->trace(
                    "  Index: {}; Priority: {}; Label: {}; Kind: {}; Text {}",
                    resIndex,
                    priority,
                    label,
                    clang_getCString(kindName),
                    getCompleteChunkKindSpelling(chunkKind));
                clang_disposeString(kindName);
            }

            completions.push_back(
                CompletionResult {
                    resIndex,
                    label,
                    detail,
                    insertText,
                    isSnippet,
                    documentation,
                    itemKind,
                    sortText,
                    isDeprecated,
                    commitChars,
                    itemTags,
                    preselect});

            // Stop after the first match.
            break;
        }
    }
    return completions;
}

std::vector<CompletionResult> Completion::GetCompletions(
    const std::string &filePath, unsigned lineno, unsigned columnno)
{
    logger()->debug("Get completions for {}:{}:{}", filePath, lineno, columnno);
    // Look up cached TU
    auto tuIt = m_translationUnits.find(filePath);
    if (tuIt == m_translationUnits.end())
    {
        logger()->error("No translation unit for {}", filePath);
        return {};
    }
    CXTranslationUnit translationUnit = tuIt->second.tu;

    // Look up cached content
    auto contentIt = m_fileContents.find(filePath);
    const std::string *contentPtr = nullptr;
    if (contentIt != m_fileContents.end())
    {
        contentPtr = &contentIt->second;
    }
    else
    {
        logger()->error("Content is not available for {}", filePath);
        return {};
    }

    unsigned completionColumn;
    auto prefix = GetPrefix(translationUnit, filePath, lineno, columnno, completionColumn);
    if (!prefix)
    {
        logger()->error("Unable to get prefix under cursor in {}", filePath);
        return {};
    }


    CXUnsavedFile unsavedFile;
    unsavedFile.Filename = filePath.c_str();
    unsavedFile.Contents = contentPtr->data();
    unsavedFile.Length = static_cast<unsigned long>(contentPtr->size());

    const unsigned flags = clang_defaultCodeCompleteOptions() | CXCodeComplete_IncludeBriefComments;
    CXCodeCompleteResults *compResults =
        clang_codeCompleteAt(translationUnit, filePath.c_str(), lineno, completionColumn, &unsavedFile, 1, flags);

    std::vector<CompletionResult> completions;
    do
    {
        if (!compResults)
        {
            logger()->trace("No completions available");
            break;
        }

        logger()->trace("Completions: {}", compResults->NumResults);
        completions = FilterCompletions(compResults, *prefix);
    } while (false);

    clang_disposeCodeCompleteResults(compResults);
    return completions;
}


std::shared_ptr<ICompletion> CreateCompletion()
{
    return std::make_shared<Completion>();
}

std::vector<std::string> BuildDefaultTranslationOptions(const std::string &clStandard)
{
    std::vector<std::string> options;
    options.push_back("-x");
    options.push_back("cl");
    if (!clStandard.empty())
    {
        options.push_back("-cl-std=" + clStandard);
    }
    return options;
}

std::string GetClangVersion()
{
    auto clangVersion = clang_getClangVersion();
    std::string result(clang_getCString(clangVersion));
    clang_disposeString(clangVersion);
    return result;
}

} // namespace ocls
