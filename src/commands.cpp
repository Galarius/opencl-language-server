//
//  commands.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/05/26.
//

#include "clinfo.hpp"
#include "diagnostics.hpp"
#include "commands.hpp"
#include "completion.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "definition.hpp"
#include "typedef.hpp"
#include "declaration.hpp"

#include <filesystem>
#include <list>

namespace fs = std::filesystem;

namespace {
auto logger()
{
    return spdlog::get(ocls::LogName::main);
}

std::list<const char*> clVersions = {
    "cl",
    "CL",
    "cl1.0",
    "CL1.0",
    "cl1.1",
    "CL1.1",
    "cl1.2",
    "CL1.2",
    "cl2.0",
    "CL2.0",
    "cl3.0",
    "CL3.0",
    "clc++",
    "CLC++",
    "clc++1.0",
    "CLC++1.0",
    "clc++2021",
    "CLC++2021"};
} // namespace

namespace ocls {

// SubCommand

SubCommand::SubCommand(CLI::App& app, std::string name, std::string description)
    : cmd {app.add_subcommand(std::move(name), std::move(description))}
{}

bool SubCommand::IsParsed() const
{
    return cmd->parsed();
}

// CLInfoSubCommand

CLInfoSubCommand::CLInfoSubCommand(CLI::App& app)
    : SubCommand(app, "clinfo", "Show information about available OpenCL devices")
{
    cmd->add_flag("-p,--pretty-print", prettyPrint, "Enable pretty-printing");
}

int CLInfoSubCommand::Execute()
{
    auto clinfo = CreateCLInfo();
    auto jsonBody = clinfo->json();
    auto indentation = prettyPrint ? 4 : -1;
    auto info = jsonBody.dump(indentation);
    logger()->trace("Result: {}", info);
    std::cout << info << std::endl;
    return EXIT_SUCCESS;
}

// DiagnosticsSubCommand

DiagnosticsSubCommand::DiagnosticsSubCommand(CLI::App& app)
    : SubCommand(app, "diagnostics", "Provides an OpenCL kernel diagnostics")
{
    cmd->add_flag("-j,--json", json, "Print diagnostics in JSON format");
    cmd->add_option("-k,--kernel", kernel, "Path to a kernel file")->required(true);
    cmd->add_option("-b,--build-options", buildOptions, "Options to be utilized when building the program.")
        ->capture_default_str();
    cmd->add_option(
           "-d,--device-id",
           deviceID,
           "Device ID or 0 (automatic selection) of the OpenCL device to be used for diagnostics.")
        ->capture_default_str();
    cmd->add_option("--error-limit", maxNumberOfProblems, "The maximum number of errors parsed by the compiler.")
        ->capture_default_str();
}

int DiagnosticsSubCommand::Execute()
{
    if (!fs::exists(kernel))
    {
        std::cerr << "Kernel file '" << kernel << "' does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    auto content = utils::ReadFileContent(kernel);
    if (!content.has_value())
    {
        return EXIT_FAILURE;
    }

    try
    {
        auto clinfo = CreateCLInfo();
        auto diagnostics = CreateDiagnostics(std::move(clinfo));

        if (deviceID > 0)
        {
            diagnostics->SetOpenCLDevice(deviceID);
        }

        if (!buildOptions.empty())
        {
            diagnostics->SetBuildOptions(buildOptions);
        }

        if (maxNumberOfProblems != INT8_MAX)
        {
            diagnostics->SetMaxProblemsCount(maxNumberOfProblems);
        }

        Source source {kernel, *content};
        if (json)
        {
            auto output = diagnostics->GetDiagnostics(source);
            std::cout << output.dump(4) << std::endl;
        }
        else
        {
            auto output = diagnostics->GetBuildLog(source);
            std::cout << output << std::endl;
        }
    }
    catch (std::exception& err)
    {
        std::cerr << "Failed to get diagnostics: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// CompletionSubCommand

CompletionSubCommand::CompletionSubCommand(CLI::App& app)
    : SubCommand(app, "completion", "Provides an OpenCL kernel completions")
{
    cmd->add_flag("-j,--json", json, "Print diagnostics in JSON format");
    cmd->add_option("-k,--kernel", kernel, "Path to a kernel file")->required(true);

    // https://clang.llvm.org/docs/ClangCommandLineReference.html
    cmd->add_option("--cl-std", clVersion, "OpenCL version")
        ->check(CLI::IsMember(clVersions))
        ->required(true)
        ->capture_default_str();
    cmd->add_option("-l,--line", line, "line number (1-based)")->required(true)->capture_default_str();
    cmd->add_option("-c,--column", column, "column number (1-based)")->required(true)->capture_default_str();
}

int CompletionSubCommand::Execute()
{
    try
    {
        auto options = BuildDefaultTranslationOptions(clVersion);
        auto store = CreateTranslationUnitStore();
        store->SaveHeaders();
        store->SetTranslationOptions(options);
        auto completion = CreateCompletion(store);

        if (!fs::exists(kernel))
        {
            std::cerr << "Kernel file '" << kernel << "' does not exist" << std::endl;
            return EXIT_FAILURE;
        }

        auto content = utils::ReadFileContent(kernel);
        if (!content.has_value())
        {
            return EXIT_FAILURE;
        }
        store->OnFileOpen(kernel, *content);
        auto completions = completion->GetCompletions(kernel, line, column);
        store->OnFileClose(kernel);

        if (json)
        {
            nlohmann::json completionItems = nlohmann::json::array();
            for (const auto& item : completions)
            {
                completionItems.emplace_back(item.toJson());
            }
            std::cout << completionItems.dump(4) << std::endl;
        }
        else
        {
            std::cout << "#" << ", "
                      << "label" << ", "
                      << "detail" << ", "
                      << "insertText" << ", "
                      << "isSnippet" << ", "
                      << "documentation" << ", "
                      << "itemKind" << ", "
                      << "sortText" << ", "
                      << "deprecated" << ", "
                      << "commitCharacters" << ", "
                      << "tags" << ", "
                      << "preselect" << std::endl;

            int index = 1;

            for (auto& item : completions)
            {
                std::string commitCharsStr = "[";
                for (const auto& ch : item.commitCharacters)
                {
                    commitCharsStr += "'" + ch + "', ";
                }
                if (!item.commitCharacters.empty())
                {
                    commitCharsStr.pop_back();
                    commitCharsStr.pop_back();
                }
                commitCharsStr += "]";

                // convert tags to string for printing
                std::string tagsStr = "[";
                for (const auto& tag : item.tags)
                {
                    tagsStr += std::to_string(tag) + ", ";
                }
                if (!item.tags.empty())
                {
                    tagsStr.pop_back();
                    tagsStr.pop_back();
                }
                tagsStr += "]";

                std::cout << index++ << ", " << item.label << ", " << item.detail << ", " << item.insertText << ", "
                          << utils::FormatBool(item.isSnippet) << ", " << item.documentation << ", "
                          << static_cast<unsigned>(item.itemKind) << ", " << item.sortText << ", "
                          << utils::FormatBool(item.deprecated) << ", " << commitCharsStr << ", " << tagsStr << ", "
                          << utils::FormatBool(item.preselect) << std::endl;
            }
            std::cout << std::endl;
        }
    }
    catch (std::exception& err)
    {
        std::cerr << "Failed to get completion: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// LocationSubCommand

LocationSubCommand::LocationSubCommand(
    CLI::App& app, std::string name, std::string description, LocationResolverFactory resolverFactory)
    : SubCommand(app, std::move(name), std::move(description))
    , m_resolverFactory {std::move(resolverFactory)}
{
    cmd->add_flag("-j,--json", json, "Print diagnostics in JSON format");
    cmd->add_option("-k,--kernel", kernel, "Path to a kernel file")->required(true);

    // https://clang.llvm.org/docs/ClangCommandLineReference.html
    cmd->add_option("--cl-std", clVersion, "OpenCL version")
        ->check(CLI::IsMember(clVersions))
        ->required(true)
        ->capture_default_str();
    cmd->add_option("-l,--line", line, "line number (1-based)")->required(true)->capture_default_str();
    cmd->add_option("-c,--column", column, "column number (1-based)")->required(true)->capture_default_str();
}

int LocationSubCommand::Execute()
{
    try
    {
        auto options = BuildDefaultTranslationOptions(clVersion);
        auto store = CreateTranslationUnitStore();
        store->SaveHeaders();
        store->SetTranslationOptions(options);
        auto resolve = m_resolverFactory(store);

        if (!fs::exists(kernel))
        {
            std::cerr << "Kernel file '" << kernel << "' does not exist" << std::endl;
            return EXIT_FAILURE;
        }

        auto content = utils::ReadFileContent(kernel);
        if (!content.has_value())
        {
            return EXIT_FAILURE;
        }
        store->OnFileOpen(kernel, *content);
        auto locations = resolve(kernel, line, column);
        store->OnFileClose(kernel);

        if (json)
        {
            nlohmann::json locationItems = nlohmann::json::array();
            for (const auto& item : locations)
            {
                locationItems.emplace_back(item.toJson(true));
            }
            std::cout << locationItems.dump(4) << std::endl;
        }
        else
        {
            PrintTable(locations);
        }
    }
    catch (std::exception& err)
    {
        std::cerr << "Failed to get " << cmd->get_name() << ": " << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void LocationSubCommand::PrintTable(const std::vector<Location>& locations)
{
    std::cout << "#" << ", "
              << "uri" << ", "
              << "startLine" << ", "
              << "startColumn" << ", "
              << "endLine" << ", "
              << "endColumn" << ", "
              << "selStartLine" << ", "
              << "selStartColumn" << ", "
              << "selEndLine" << ", "
              << "selEndColumn" << std::endl;

    int index = 1;
    for (const auto& item : locations)
    {
        std::cout << index++ << ", " << item.uri << ", " << item.startLine << ", " << item.startColumn << ", "
                  << item.endLine << ", " << item.endColumn << ", " << item.selStartLine << ", " << item.selStartColumn
                  << ", " << item.selEndLine << ", " << item.selEndColumn << std::endl;
    }
    std::cout << std::endl;
}

std::shared_ptr<LocationSubCommand> MakeDefinitionSubCommand(CLI::App& app)
{
    return std::make_shared<LocationSubCommand>(
        app,
        "definition",
        "Resolves the definition location of a symbol at a given text document position",
        [](std::shared_ptr<ITranslationUnitStore> store) -> LocationResolver {
            auto definition = CreateDefinition(store);
            return [definition](const std::string& filePath, unsigned line, unsigned column) {
                return definition->GetDefinitions(filePath, line, column);
            };
        });
}

std::shared_ptr<LocationSubCommand> MakeDeclarationSubCommand(CLI::App& app)
{
    return std::make_shared<LocationSubCommand>(
        app,
        "declaration",
        "Resolves the declaration location of a symbol at a given text document position",
        [](std::shared_ptr<ITranslationUnitStore> store) -> LocationResolver {
            auto declaration = CreateDeclaration(store);
            return [declaration](const std::string& filePath, unsigned line, unsigned column) {
                return declaration->GetDeclarations(filePath, line, column);
            };
        });
}

std::shared_ptr<LocationSubCommand> MakeTypeDefinitionSubCommand(CLI::App& app)
{
    return std::make_shared<LocationSubCommand>(
        app,
        "typedef",
        "Resolves the type definition location of a symbol at a given text document position",
        [](std::shared_ptr<ITranslationUnitStore> store) -> LocationResolver {
            auto typeDefinition = CreateTypeDefinition(store);
            return [typeDefinition](const std::string& filePath, unsigned line, unsigned column) {
                return typeDefinition->GetTypeDefinitions(filePath, line, column);
            };
        });
}

} // namespace ocls
