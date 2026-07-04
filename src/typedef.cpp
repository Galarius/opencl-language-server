//
//  typedef.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "typedef.hpp"
#include "log.hpp"

#include <clang-c/Index.h>

namespace {

auto logger()
{
    return spdlog::get(ocls::LogName::typeDefinition);
}

/**
 Strip pointer/reference layers so `__global Vec3 *positions` resolves to
 `Vec3`, not to a pointer-to-Vec3 pseudo-type with no declaration.
 */
CXType stripIndirection(CXType type)
{
    CXType current = type;
    while (current.kind == CXType_Pointer || current.kind == CXType_LValueReference ||
           current.kind == CXType_RValueReference)
    {
        current = clang_getPointeeType(current);
    }
    return current;
}

/**
 If the resolved declaration is itself a typedef (e.g. `typedef struct {...} Vec3;`
 where the cursor kind is CXCursor_TypedefDecl), follow through to the
 underlying struct/enum/union declaration when one exists, since that's
 usually the more useful jump target for "go to type definition".
 */
CXCursor resolveThroughTypedef(CXCursor declCursor)
{
    if (clang_getCursorKind(declCursor) != CXCursor_TypedefDecl)
    {
        return declCursor;
    }

    CXType underlyingType = clang_getTypedefDeclUnderlyingType(declCursor);
    CXCursor underlyingDecl = clang_getTypeDeclaration(underlyingType);
    if (clang_Cursor_isNull(underlyingDecl))
    {
        return declCursor;
    }
    return underlyingDecl;
}

} // namespace

namespace ocls {

class TypeDefinition final : public ITypeDefinition
{
public:
    explicit TypeDefinition(std::shared_ptr<ITranslationUnitStore> store) 
        : m_store(std::move(store)) {}

    std::vector<Location> GetTypeDefinitions(const std::string &filePath, unsigned lineno, unsigned columnno) override;

private:
    std::shared_ptr<ITranslationUnitStore> m_store;
};

std::vector<Location> TypeDefinition::GetTypeDefinitions(
    const std::string &filePath, unsigned lineno, unsigned columnno)
{
    logger()->debug("Get type definition for {}:{}:{}", filePath, lineno, columnno);

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

    CXType type = clang_getCursorType(cursor);
    type = stripIndirection(type);

    CXCursor typeDecl = clang_getTypeDeclaration(type);
    if (clang_Cursor_isNull(typeDecl))
    {
        logger()->debug("No declaration found for type of symbol at {}:{}:{}", filePath, lineno, columnno);
        return {};
    }

    typeDecl = resolveThroughTypedef(typeDecl);

    auto location = MakeLocation(typeDecl);
    if (!location)
    {
        logger()->debug("Unable to resolve type definition location for {}:{}:{}", filePath, lineno, columnno);
        return {};
    }

    return {*location};
}

std::shared_ptr<ITypeDefinition> CreateTypeDefinition(std::shared_ptr<ITranslationUnitStore> store)
{
    return std::make_shared<TypeDefinition>(std::move(store));
}

} // namespace ocls
