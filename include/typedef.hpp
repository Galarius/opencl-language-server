//
//  typedef.hpp
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
 Provides `textDocument/typeDefinition` results: given a symbol position,
 resolves to where the *type* of that symbol is defined (e.g. jumping
 from a variable to the struct/typedef that defines its shape), as
 opposed to `textDocument/definition` which resolves the symbol itself.
 */
struct ITypeDefinition
{
    virtual ~ITypeDefinition() = default;

    /**
     \note \c lineno / \c columnno are 1-based values
     */
    virtual std::vector<Location> GetTypeDefinitions(
        const std::string &filePath, unsigned lineno, unsigned columnno) = 0;
};

std::shared_ptr<ITypeDefinition> CreateTypeDefinition(std::shared_ptr<ITranslationUnitStore> store);

} // namespace ocls