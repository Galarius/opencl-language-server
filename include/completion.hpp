//
//  completion.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 8/30/23.
//

#pragma once

#include "translation.hpp"

#include <clinfo.hpp>

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace ocls {

enum class CompletionItemKind : unsigned
{
    unknown = 0,
    // should match
    // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionItemKind
    text = 1,
    method = 2,
    function = 3,
    constructor = 4,
    field = 5,
    variable = 6,
    class_ = 7,
    interface = 8,
    module_ = 9,
    property = 10,
    unit = 11,
    value = 12,
    enum_ = 13,
    keyword = 14,
    snippet = 15,
    color = 16,
    file = 17,
    reference = 18,
    folder = 19,
    enumMember = 20,
    constant = 21,
    struct_ = 22,
    event = 23,
    operator_ = 24,
    typeParameter = 25
    //
};

enum class InsertTextFormat : unsigned
{
    /**
     * The primary text to be inserted is treated as a plain string.
     */
    plainText = 1,
    /**
     * The primary text to be inserted is treated as a snippet.
     *
     * A snippet can define tab stops and placeholders with `$1`, `$2`
     * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
     * the end of the snippet. Placeholders with equal identifiers are linked,
     * that is typing in one will update others too.
     */
    snippet = 2
};

struct CompletionResult
{
    unsigned index;
    std::string label;
    std::string detail;     // type or symbol information
    std::string insertText; // text to insert (may include snippets)
    bool isSnippet = false;
    std::string documentation; // doc comments or hover info
    CompletionItemKind itemKind = CompletionItemKind::unknown;
    std::string sortText;                      // lexicographic sort key derived from priority
    bool deprecated = false;                   // marked as deprecated by clang
    std::vector<std::string> commitCharacters; // characters that auto-commit
    std::vector<unsigned> tags;                // LSP tags: 1=Deprecated, 2=Uncomplete
    bool preselect = false;                    // preselect this item

    std::string makeKey() const
    {
        return CompletionResult::makeKey(label, index);
    }

    static std::string makeKey(const std::string label, unsigned index)
    {
        return label + std::to_string(index);
    }

    nlohmann::json toJsonIncomplete() const
    {
        // Return basic completion item for quick display
        return nlohmann::json {
            {"label", label},
            {"detail", detail},
            {"kind", static_cast<unsigned>(itemKind)},
            {"sortText", sortText},
            // Will be preserved in textDocument/resolve request
            {"data", {{"index", index}}}};
    }

    nlohmann::json toJson() const
    {
        // Return full completion item for resolution
        auto json = toJsonIncomplete();
        json["documentation"] = documentation;
        json["insertText"] = insertText;
        json["insertTextFormat"] = isSnippet ? InsertTextFormat::snippet : InsertTextFormat::plainText;
        json["deprecated"] = deprecated;
        json["commitCharacters"] = commitCharacters;
        json["tags"] = tags;
        json["preselect"] = preselect;
        return json;
    }
};

struct ICompletion
{
    virtual ~ICompletion() = default;

    /**
     * \note \c line and \c column are 1-based values
     */
    virtual std::vector<CompletionResult> GetCompletions(
        const std::string &filePath, unsigned line, unsigned column) = 0;
};

std::shared_ptr<ICompletion> CreateCompletion(std::shared_ptr<ITranslationUnitStore> store);

} // namespace ocls
