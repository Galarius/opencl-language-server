//
//  translation.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include <clang-c/Index.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ocls {

/**
 Owns the lifecycle of libclang translation units and their backing file
 content, plus the shared header cache and translation options.
 */
struct ITranslationUnitStore
{
    virtual ~ITranslationUnitStore() = default;

    virtual void OnFileOpen(const std::string &filePath, const std::string &content) = 0;
    virtual void OnFileChange(const std::string &filePath, const std::string &content) = 0;
    virtual void OnFileClose(const std::string &filePath) = 0;

    /**
     * Set the command-line arguments that would be
     * passed to the \c clang executable if it were being invoked out-of-process.
     * These command-line options will be parsed and will affect how the translation
     * unit is parsed. Note that the following options are ignored: '-c',
     * '-emit-ast', '-fsyntax-only' (which is the default), and '-o \<output file>'.
     *
     * \see BuildDefaultTranslationOptions
     */
    virtual void SetTranslationOptions(const std::vector<std::string> &options) = 0;

    /**
     * Saves embedded OpenCL headers to disk
     */
    virtual void SaveHeaders() = 0;

    /**
     * Returns the cached translation unit for \c filePath, or \c nullptr
     * if the file hasn't been opened / parsing failed.
     */
    virtual CXTranslationUnit GetTranslationUnit(const std::string &filePath) const = 0;

    /**
     * Returns a pointer to the cached in-memory content for \c filePath,
     * or \c nullptr if not tracked. The pointer is valid until the next
     * \c OnFileChange / \c OnFileClose call for the same file.
     */
    virtual const std::string *GetContent(const std::string &filePath) const = 0;

    /**
     * Convenience helper: resolves a \c CXCursor at the given 1-based
     * line/column in \c filePath using the cached translation unit.
     * Returns a null cursor (\c clang_Cursor_isNull) if the TU is missing
     * or the file cannot be located within it.
     */
    virtual CXCursor GetCursorAt(const std::string &filePath, unsigned lineno, unsigned columnno) const = 0;
};

std::shared_ptr<ITranslationUnitStore> CreateTranslationUnitStore();

std::vector<std::string> BuildDefaultTranslationOptions(const std::string &clStandard);

std::string GetClangVersion();

} // namespace ocls
