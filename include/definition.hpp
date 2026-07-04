//
//  definition.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "translation.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocls {

/**
 Represents a single resolved location, matching the fields needed to
 build either an LSP `Location` or `LocationLink`.

 line/column values are 0-based, matching LSP's convention.
 */
struct DefinitionLocation
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
 Provides `textDocument/definition` results: given a symbol position
 (e.g. a function prototype), returns the location(s) of its concrete
 definition(s).
 */
struct IDefinition
{
    virtual ~IDefinition() = default;

    /**
     \note \c lineno / \c columnno are 1-based values
     */
    virtual std::vector<DefinitionLocation> GetDefinitions(
        const std::string &filePath, unsigned lineno, unsigned columnno) = 0;
};

std::shared_ptr<IDefinition> CreateDefinition(std::shared_ptr<ITranslationUnitStore> store);

} // namespace ocls
