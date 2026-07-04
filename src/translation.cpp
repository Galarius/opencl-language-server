//
//  translation.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "translation.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <clang-c/Index.h>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

// Generated
#include "embedded_headers.h"

namespace fs = std::filesystem;

namespace {

constexpr int excludeDeclsFromPCH = 0;
constexpr int displayDiagnostics = 0;

auto logger()
{
    return spdlog::get(ocls::LogName::translation);
}

std::unordered_map<CXErrorCode, std::string> translationErrorSpellingMap = {
    {CXError_Failure, "Failure"},
    {CXError_Crashed, "Crashed"},
    {CXError_InvalidArguments, "InvalidArguments"},
    {CXError_ASTReadError, "ASTReadError"},
    {CXError_Success, "Success"}};

struct TranslationUnitEntry
{
    CXTranslationUnit tu;
    CXIndex index;
};

} // namespace

namespace ocls {

class TranslationUnitStore final : public ITranslationUnitStore
{
public:
    ~TranslationUnitStore() override
    {
        DestroyTranslationUnits();
        DeleteCache();
    }

    void OnFileOpen(const std::string &filePath, const std::string &content) override;
    void OnFileChange(const std::string &filePath, const std::string &content) override;
    void OnFileClose(const std::string &filePath) override;
    void SetTranslationOptions(const std::vector<std::string> &options) override;

    void SaveHeaders() override;

    CXTranslationUnit GetTranslationUnit(const std::string &filePath) const override;
    const std::string *GetContent(const std::string &filePath) const override;
    CXCursor GetCursorAt(const std::string &filePath, unsigned lineno, unsigned columnno) const override;

private:
    void DestroyTranslationUnits() noexcept;
    void DeleteCache() noexcept;

    std::vector<CXUnsavedFile> BuildUnsavedPool(const std::string &filePath, const std::string &content);

private:
    std::optional<fs::path> m_cacheDir;
    std::optional<fs::path> m_headersDir;
    std::vector<std::string> m_args;
    std::unordered_map<std::string, std::string> m_fileContents;
    std::unordered_map<std::string, TranslationUnitEntry> m_translationUnits;
};

void TranslationUnitStore::DestroyTranslationUnits() noexcept
{
    try
    {
        logger()->trace("TranslationUnitStore::DestroyTranslationUnits");
        for (auto &[filePath, entry] : m_translationUnits)
        {
            m_fileContents.erase(filePath);
            clang_disposeTranslationUnit(entry.tu);
            clang_disposeIndex(entry.index);
        }
        m_translationUnits.clear();
    }
    catch (...)
    {}
}

void TranslationUnitStore::SaveHeaders()
{
#if defined(__APPLE__)
    auto temp = fs::temp_directory_path() / "com.galarius.opencl-language-server";
#else
    auto temp = fs::temp_directory_path() / "opencl-language-server";
#endif
    auto headersDir = temp / version / "headers";

    logger()->debug("Using cache dir: {}", temp.string());
    fs::create_directories(headersDir);
    // Unpack embedded headers onto the disk if they don't exist yet
    const auto &header_map = resources::get_headers();
    for (const auto &[filename, contents] : header_map)
    {
        fs::path targetFilePath = fs::path(headersDir) / filename;
        if (!fs::exists(targetFilePath))
        {
            std::ofstream file(targetFilePath, std::ios::binary);
            if (file.is_open())
            {
                file.write(contents.data(), contents.size());
            }
        }
    }

    m_cacheDir = temp;
    m_headersDir = headersDir;
}

void TranslationUnitStore::DeleteCache() noexcept
{
    try
    {
        if (m_cacheDir && fs::exists(*m_cacheDir))
        {
            auto cacheDir = m_cacheDir.value();
            logger()->debug("Deleting cache dir {}", cacheDir.string());
            fs::remove_all(cacheDir);
        }
    }
    catch (...)
    {}
}

std::vector<CXUnsavedFile> TranslationUnitStore::BuildUnsavedPool(
    const std::string &filePath, const std::string &content)
{
    std::vector<CXUnsavedFile> unsavedPool;
    unsavedPool.resize(1);
    unsavedPool[0].Filename = filePath.c_str();
    unsavedPool[0].Contents = content.data();
    unsavedPool[0].Length = static_cast<unsigned long>(content.size());
    return unsavedPool;
}

void TranslationUnitStore::OnFileOpen(const std::string &filePath, const std::string &content)
{
    logger()->trace("TranslationUnitStore::OnFileOpen - {}", filePath);
    if (!fs::exists(filePath))
    {
        logger()->error("File does not exist: {}", filePath);
        return;
    }

    m_fileContents[filePath] = content;

    CXIndex index = clang_createIndex(excludeDeclsFromPCH, displayDiagnostics);

    auto unsavedFiles = BuildUnsavedPool(filePath, content);
    unsigned options = clang_defaultEditingTranslationUnitOptions();
    options |= CXTranslationUnit_SkipFunctionBodies;
    options |= CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;
    

    std::vector<const char *> cargs;
    cargs.reserve(m_args.size());
    for (const auto &arg : m_args)
    {
        cargs.push_back(arg.c_str());
    }

    CXTranslationUnit tu;
    CXErrorCode code = clang_parseTranslationUnit2(
        index,
        filePath.c_str(),
        cargs.data(),
        static_cast<int>(cargs.size()),
        unsavedFiles.data(),
        static_cast<unsigned int>(unsavedFiles.size()),
        options,
        &tu);

    if (code != CXError_Success)
    {
        std::string error = translationErrorSpellingMap[code];
        logger()->error("Parsing failed with error code: {}", error);
        clang_disposeIndex(index);
        return;
    }

    m_translationUnits[filePath] = {tu, index};
}

void TranslationUnitStore::OnFileChange(const std::string &filePath, const std::string &content)
{
    logger()->trace("TranslationUnitStore::OnFileChange - {}", filePath);
    m_fileContents[filePath] = content;

    auto it = m_translationUnits.find(filePath);
    if (it == m_translationUnits.end())
    {
        logger()->debug("TU does not exist, performing full parse...");
        // No TU yet, do a full parse
        OnFileOpen(filePath, content);
        return;
    }

    auto unsavedFiles = BuildUnsavedPool(filePath, content);
    const unsigned options = clang_defaultReparseOptions(it->second.tu);

    // Reparse is much cheaper than a full parse —
    // reuses the precompiled preamble if it hasn't changed
    int result = clang_reparseTranslationUnit(
        it->second.tu, static_cast<unsigned int>(unsavedFiles.size()), unsavedFiles.data(), options);

    if (result != 0)
    {
        CXErrorCode code = static_cast<CXErrorCode>(result);
        std::string error = translationErrorSpellingMap[code];
        logger()->error("Failed to reparse TU: {}", error);
        clang_disposeTranslationUnit(it->second.tu);
        clang_disposeIndex(it->second.index);
        m_translationUnits.erase(it);
        OnFileOpen(filePath, content);
    }
}

void TranslationUnitStore::OnFileClose(const std::string &filePath)
{
    logger()->trace("TranslationUnitStore::OnFileClose - {}", filePath);
    m_fileContents.erase(filePath);

    auto it = m_translationUnits.find(filePath);
    if (it != m_translationUnits.end())
    {
        clang_disposeTranslationUnit(it->second.tu);
        clang_disposeIndex(it->second.index);
        m_translationUnits.erase(it);
    }
}

void TranslationUnitStore::SetTranslationOptions(const std::vector<std::string> &options)
{
    m_args.clear();

    for (const auto &arg : options)
    {
        m_args.push_back(arg);
    }

    if (m_headersDir)
    {
        m_args.push_back("-I" + m_headersDir.value().string());
        const auto &headers = resources::get_headers();
        for (const auto &[filename, _] : headers)
        {
            m_args.push_back("-include");
            m_args.push_back(filename);
        }
    }

    logger()->debug("TranslationUnitStore::SetTranslationOptions - {}", utils::FormatVector(m_args));

    // We need to re-parse sources with new options
    DestroyTranslationUnits();
}

CXTranslationUnit TranslationUnitStore::GetTranslationUnit(const std::string &filePath) const
{
    auto it = m_translationUnits.find(filePath);
    if (it == m_translationUnits.end())
    {
        return nullptr;
    }
    return it->second.tu;
}

const std::string *TranslationUnitStore::GetContent(const std::string &filePath) const
{
    auto it = m_fileContents.find(filePath);
    if (it == m_fileContents.end())
    {
        return nullptr;
    }
    return &it->second;
}

CXCursor TranslationUnitStore::GetCursorAt(const std::string &filePath, unsigned lineno, unsigned columnno) const
{
    CXTranslationUnit tu = GetTranslationUnit(filePath);
    if (!tu)
    {
        return clang_getNullCursor();
    }

    CXFile file = clang_getFile(tu, filePath.c_str());
    if (!file)
    {
        logger()->error("Unable to get file for {}", filePath);
        return clang_getNullCursor();
    }

    CXSourceLocation loc = clang_getLocation(tu, file, lineno, columnno);
    return clang_getCursor(tu, loc);
}

std::shared_ptr<ITranslationUnitStore> CreateTranslationUnitStore()
{
    return std::make_shared<TranslationUnitStore>();
}

std::vector<std::string> BuildDefaultTranslationOptions(const std::string &clStandard)
{
    std::vector<std::string> options;
    options.push_back("-x");
    options.push_back("cl");
    if (!clStandard.empty())
    {
        options.push_back("-cl-std=" + clStandard);
    }
    return options;
}

std::string GetClangVersion()
{
    auto clangVersion = clang_getClangVersion();
    std::string result(clang_getCString(clangVersion));
    clang_disposeString(clangVersion);
    return result;
}

} // namespace ocls
