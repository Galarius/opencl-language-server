//
//  location.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include <clang-c/Index.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace ocls {

/**
 Represents a single resolved location, matching the fields needed to
 build either an LSP `Location` or `LocationLink`.

 line/column values are 0-based, matching LSP's convention.
 */
struct Location
{
    std::string uri;
    unsigned startLine;
    unsigned startColumn;
    unsigned endLine;
    unsigned endColumn;
    // Narrower "selection" range (usually just the identifier),
    // used for `targetSelectionRange` in `LocationLink`.
    unsigned selStartLine;
    unsigned selStartColumn;
    unsigned selEndLine;
    unsigned selEndColumn;

    nlohmann::json toJson(bool linkSupport) const
    {
        if (linkSupport)
        {
            return nlohmann::json {
                {"targetUri", uri},
                {"targetRange",
                 {{"start", {{"line", startLine}, {"character", startColumn}}},
                  {"end", {{"line", endLine}, {"character", endColumn}}}}},
                {"targetSelectionRange",
                 {{"start", {{"line", selStartLine}, {"character", selStartColumn}}},
                  {"end", {{"line", selEndLine}, {"character", selEndColumn}}}}}};
        }
        else
        {
            return nlohmann::json {
                {"uri", uri},
                {"range",
                 {{"start", {{"line", selStartLine}, {"character", selStartColumn}}},
                  {"end", {{"line", selEndLine}, {"character", selEndColumn}}}}}};
        }
    }
};

/**
 Build a \c Location from a cursor, splitting the "full" extent (e.g. the
 whole function/struct body) from the narrower "selection" extent (just
 the identifier), matching what LocationLink needs. Returns \c std::nullopt
 if the cursor is null or its file cannot be resolved.
 */
std::optional<Location> MakeLocation(CXCursor cursor);

} // namespace ocls