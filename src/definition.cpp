//
//  definition.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "definition.hpp"
#include "log.hpp"

#include <clang-c/Index.h>
#include <cstring>
#include <memory>
#include <vector>

namespace {

auto logger()
{
    return spdlog::get(ocls::LogName::definition);
}

/**
 Build a location record from a cursor, splitting the "full" extent
 (e.g. the whole function body) from the narrower "selection" extent
 (just the identifier), matching what LocationLink needs.
 */
std::optional<ocls::DefinitionLocation> makeLocation(CXCursor cursor)
{
    if (clang_Cursor_isNull(cursor))
    {
        return std::nullopt;
    }

    CXSourceRange fullRange = clang_getCursorExtent(cursor);
    CXSourceLocation nameLoc = clang_getCursorLocation(cursor);

    CXFile file;
    unsigned startLine, startCol, endLine, endCol;
    clang_getSpellingLocation(clang_getRangeStart(fullRange), &file, &startLine, &startCol, nullptr);
    clang_getSpellingLocation(clang_getRangeEnd(fullRange), nullptr, &endLine, &endCol, nullptr);

    if (!file)
    {
        return std::nullopt;
    }

    CXString fileNameStr = clang_getFileName(file);
    const char *fileNameC = clang_getCString(fileNameStr);
    if (!fileNameC)
    {
        clang_disposeString(fileNameStr);
        return std::nullopt;
    }
    std::string filePath = fileNameC;
    clang_disposeString(fileNameStr);

    unsigned selLine, selCol;
    clang_getSpellingLocation(nameLoc, nullptr, &selLine, &selCol, nullptr);

    CXString spelling = clang_getCursorSpelling(cursor);
    const char *spellingCStr = clang_getCString(spelling);
    unsigned nameLen = spellingCStr ? static_cast<unsigned>(strlen(spellingCStr)) : 0;
    clang_disposeString(spelling);

    ocls::DefinitionLocation loc;
    loc.uri = filePath;
    // Convert from libclang's 1-based to LSP's 0-based.
    loc.startLine = startLine > 0 ? startLine - 1 : 0;
    loc.startColumn = startCol > 0 ? startCol - 1 : 0;
    loc.endLine = endLine > 0 ? endLine - 1 : 0;
    loc.endColumn = endCol > 0 ? endCol - 1 : 0;
    loc.selStartLine = selLine > 0 ? selLine - 1 : 0;
    loc.selStartColumn = selCol > 0 ? selCol - 1 : 0;
    loc.selEndLine = loc.selStartLine;
    loc.selEndColumn = loc.selStartColumn + nameLen;

    return loc;
}

struct VisitorContext
{
    CXCursor target;
    std::vector<CXCursor> *results;
};

CXChildVisitResult visitForDefinitions(CXCursor cursor, CXCursor /*parent*/, CXClientData clientData)
{
    auto *ctx = static_cast<VisitorContext *>(clientData);
    if (clang_isCursorDefinition(cursor))
    {
        // Compare canonical cursors so a definition matches its
        // corresponding (possibly separate) declaration/prototype.
        CXCursor canonicalCursor = clang_getCanonicalCursor(cursor);
        CXCursor canonicalTarget = clang_getCanonicalCursor(ctx->target);
        if (clang_equalCursors(canonicalCursor, canonicalTarget))
        {
            ctx->results->push_back(cursor);
        }
    }

    return CXChildVisit_Recurse;
}

} // namespace

namespace ocls {

class Definition final : public IDefinition
{
public:
    explicit Definition(std::shared_ptr<ITranslationUnitStore> store) : m_store(std::move(store)) {}

    std::vector<DefinitionLocation> GetDefinitions(
        const std::string &filePath, unsigned lineno, unsigned columnno) override;

private:
    std::vector<CXCursor> FindDefinitions(CXTranslationUnit tu, CXCursor target);

private:
    std::shared_ptr<ITranslationUnitStore> m_store;
};

std::vector<CXCursor> Definition::FindDefinitions(CXTranslationUnit tu, CXCursor target)
{
    std::vector<CXCursor> results;
    VisitorContext ctx {target, &results};

    CXCursor root = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(root, visitForDefinitions, &ctx);

    return results;
}

std::vector<DefinitionLocation> Definition::GetDefinitions(
    const std::string &filePath, unsigned lineno, unsigned columnno)
{
    logger()->debug("Get definitions for {}:{}:{}", filePath, lineno, columnno);

    CXTranslationUnit translationUnit = m_store->GetTranslationUnit(filePath);
    if (!translationUnit)
    {
        logger()->error("No translation unit for {}", filePath);
        return {};
    }
    CXFile file = clang_getFile(translationUnit, filePath.c_str());
    if (!file)
    {
        logger()->error("Unable to get file for {}", filePath);
        return {};
    }

    CXSourceLocation loc = clang_getLocation(translationUnit, file, lineno, columnno);
    CXCursor cursor = clang_getCursor(translationUnit, loc);
    if (clang_Cursor_isNull(cursor))
    {
        logger()->debug("No cursor found at {}:{}:{}", filePath, lineno, columnno);
        return {};
    }

    // If we're sitting on a reference/use, resolve to the declaration first.
    CXCursor declCursor = clang_getCursorReferenced(cursor);
    if (clang_Cursor_isNull(declCursor))
    {
        declCursor = cursor;
    }

    // If the cursor we resolved to is already a definition, that's
    // the only "definition" - return it directly.
    std::vector<CXCursor> implCursors;
    if (clang_isCursorDefinition(declCursor))
    {
        implCursors.push_back(declCursor);
    }
    else
    {
        implCursors = FindDefinitions(translationUnit, declCursor);
    }

    std::vector<DefinitionLocation> locations;
    locations.reserve(implCursors.size());
    for (const auto &implCursor : implCursors)
    {
        auto location = makeLocation(implCursor);
        if (location)
        {
            locations.push_back(std::move(*location));
        }
    }

    if (locations.empty())
    {
        logger()->debug("No definitions found for {}:{}:{}", filePath, lineno, columnno);
    }

    return locations;
}

std::shared_ptr<IDefinition> CreateDefinition(std::shared_ptr<ITranslationUnitStore> store)
{
    return std::make_shared<Definition>(std::move(store));
}

} // namespace ocls
