//
//  location.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "location.hpp"

#include <cstring>

namespace ocls {

std::optional<Location> MakeLocation(CXCursor cursor)
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

    Location loc;
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

} // namespace ocls