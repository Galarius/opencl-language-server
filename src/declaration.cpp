//
//  declaration.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "declaration.hpp"
#include "log.hpp"

#include <clang-c/Index.h>

namespace {

auto logger()
{
    return spdlog::get(ocls::LogName::declaration);
}

} // namespace

namespace ocls {

class Declaration final : public IDeclaration
{
public:
    explicit Declaration(std::shared_ptr<ITranslationUnitStore> store) : m_store(std::move(store)) {}

    std::vector<Location> GetDeclarations(const std::string &filePath, unsigned lineno, unsigned columnno) override;

private:
    std::shared_ptr<ITranslationUnitStore> m_store;
};

std::vector<Location> Declaration::GetDeclarations(const std::string &filePath, unsigned lineno, unsigned columnno)
{
    logger()->debug("Get declaration for {}:{}:{}", filePath, lineno, columnno);

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

    // Resolve a use/reference to what it refers to. For a macro expansion
    // this already yields the CXCursor_MacroDefinition cursor. For a
    // function/variable use it yields whichever declaration cursor the
    // AST happened to attach the reference to (may be the definition).
    CXCursor referenced = clang_getCursorReferenced(cursor);
    if (clang_Cursor_isNull(referenced))
    {
        referenced = cursor;
    }

    // "Declaration" should prefer the first-seen prototype/forward-decl
    // over a body-bearing definition. clang_getCanonicalCursor gives us
    // exactly that for entities with multiple declarations (e.g. a
    // function prototyped in a header and defined in a .cl file).
    // For macros and other single-cursor entities this is a no-op.
    CXCursor canonical = clang_getCanonicalCursor(referenced);
    CXCursor target = clang_Cursor_isNull(canonical) ? referenced : canonical;

    auto location = MakeLocation(target);
    if (!location)
    {
        logger()->debug("Unable to resolve declaration location for {}:{}:{}", filePath, lineno, columnno);
        return {};
    }

    return {*location};
}

std::shared_ptr<IDeclaration> CreateDeclaration(std::shared_ptr<ITranslationUnitStore> store)
{
    return std::make_shared<Declaration>(std::move(store));
}

} // namespace ocls