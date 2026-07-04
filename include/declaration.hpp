//
//  declaration.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "location.hpp"
#include "translation.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ocls {

/**
 Provides `textDocument/declaration` results: given a symbol position,
 resolves to its declaration (e.g. a function prototype in a header, or
 a macro's `#define`), as distinct from `textDocument/definition` which
 resolves to the body/value.
 */
struct IDeclaration
{
    virtual ~IDeclaration() = default;

    /**
     \note \c lineno / \c columnno are 1-based values
     */
    virtual std::vector<Location> GetDeclarations(const std::string &filePath, unsigned lineno, unsigned columnno) = 0;
};

std::shared_ptr<IDeclaration> CreateDeclaration(std::shared_ptr<ITranslationUnitStore> store);

} // namespace ocls
