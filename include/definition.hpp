//
//  definition.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "translation.hpp"
#include "location.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ocls {

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
    virtual std::vector<Location> GetDefinitions(
        const std::string &filePath, unsigned lineno, unsigned columnno) = 0;
};

std::shared_ptr<IDefinition> CreateDefinition(std::shared_ptr<ITranslationUnitStore> store);

} // namespace ocls
