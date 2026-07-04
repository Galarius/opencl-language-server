//
//  commands.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/05/26.
//

#include "translation.hpp"
#include "location.hpp"

#include <memory>
#include <functional>
#include <string>
#include <cstdint>
#include <CLI/CLI.hpp>

namespace ocls {

// SubCommand

struct SubCommand
{
    SubCommand(CLI::App& app, std::string name, std::string description);
    virtual ~SubCommand() = default;
    bool IsParsed() const;

    virtual int Execute() = 0;

protected:
    CLI::App* cmd;
};

// CLInfoSubCommand

struct CLInfoSubCommand final : public SubCommand
{
    explicit CLInfoSubCommand(CLI::App& app);

    int Execute() override;

private:
    bool prettyPrint = false;
};

// DiagnosticsSubCommand

struct DiagnosticsSubCommand final : public SubCommand
{
    explicit DiagnosticsSubCommand(CLI::App& app);

    int Execute();

private:
    std::string kernel;
    std::string buildOptions;
    uint32_t deviceID = 0;
    uint64_t maxNumberOfProblems = INT8_MAX;
    bool json = false;
};

// CompletionSubCommand

struct CompletionSubCommand final : public SubCommand
{
    explicit CompletionSubCommand(CLI::App& app);

    int Execute() override;

private:
    std::string kernel;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string clVersion;
    bool json = false;
};

// LocationSubCommand

/**
 Signature shared by IDefinition::GetDefinitions, IDeclaration::GetDeclarations,
 and ITypeDefinition::GetTypeDefinitions.
 */
using LocationResolver =
    std::function<std::vector<Location>(const std::string& filePath, unsigned line, unsigned column)>;

/**
 A factory that, given a translation unit store, builds a LocationResolver
 bound to a specific feature (definition/declaration/typeDefinition/...).
 */
using LocationResolverFactory = std::function<LocationResolver(std::shared_ptr<ITranslationUnitStore>)>;

struct LocationSubCommand final : public SubCommand
{
    LocationSubCommand(
        CLI::App& app, std::string name, std::string description, LocationResolverFactory resolverFactory);

    int Execute() override;

private:
    static void PrintTable(const std::vector<Location>& locations);

private:
    std::string kernel;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string clVersion;
    bool json = false;
    LocationResolverFactory m_resolverFactory;
};

std::shared_ptr<LocationSubCommand> MakeDefinitionSubCommand(CLI::App& app);
std::shared_ptr<LocationSubCommand> MakeDeclarationSubCommand(CLI::App& app);
std::shared_ptr<LocationSubCommand> MakeTypeDefinitionSubCommand(CLI::App& app);

} // namespace ocls
