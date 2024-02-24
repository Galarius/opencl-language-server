//
//  completion.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 8/30/23.
//

#pragma once

#include <clinfo.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ocls {

enum class CompletionItemKind : unsigned {
    unknown = 0,
    // should match https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionItemKind
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

struct CompletionResult
{
    std::string label;
    std::string detail; // type or symbol information
    CompletionItemKind itemKind = CompletionItemKind::unknown;
    unsigned priority = 0;
};

struct ICompletion
{
    virtual ~ICompletion() = default;

    virtual std::vector<CompletionResult> GetCompletions(const std::string &fileName, unsigned line, unsigned column) = 0;
};

std::shared_ptr<ICompletion> CreateCompletion(std::vector<std::string> args);

} // namespace ocls
