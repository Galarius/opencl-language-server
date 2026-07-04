//
//  definition-tests.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "definition.hpp"
#include "translation.hpp"
#include "log.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace ocls;
using namespace testing;

namespace fs = std::filesystem;

namespace {

const std::string TEST_FIXTURE_DIR = fs::path(__FILE__).parent_path().string() + "/fixtures";
const std::string KERNEL_FILE = TEST_FIXTURE_DIR + "/kernel.cl";
// Line 6: "int getChannel(uint rgb, int channel);"
// Position 8 is after "get" to trigger completion for getChannel
const unsigned line = 6;
const unsigned column = 8;

class DefinitionTest : public ::testing::Test
{
protected:
    std::shared_ptr<IDefinition> definition;
    std::shared_ptr<ITranslationUnitStore> store;

    void SetUp() override
    {
        std::string fileContent;
        std::ifstream f(KERNEL_FILE);
        if (f.is_open())
        {
            fileContent.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        }
        auto options = BuildDefaultTranslationOptions("cl1.2");
        store = CreateTranslationUnitStore();
        store->SaveHeaders(); // without opencl-c.h, the 'GetDefinitions' cascades into broken cursor resolution
        store->SetTranslationOptions(options);
        definition = CreateDefinition(store);
        
        store->OnFileOpen(KERNEL_FILE, fileContent);
    }
    
    void TearDown() override
    {
        store->OnFileClose(KERNEL_FILE);
    }
};

} // namespace

// Test basic definition at a function call site
TEST_F(DefinitionTest, DefinitionAtFunctionCall)
{
    auto results = definition->GetDefinitions(KERNEL_FILE, line, column);
    ASSERT_GT(results.size(), 0);

    auto r = results.front();
    ASSERT_EQ(r.uri, KERNEL_FILE);
    ASSERT_EQ(r.startLine, 20);
    ASSERT_EQ(r.startColumn, 0);
    ASSERT_EQ(r.endLine, 31);
    ASSERT_EQ(r.endColumn, 1);
    ASSERT_EQ(r.selStartLine, 20);
    ASSERT_EQ(r.selStartColumn, 4);
    ASSERT_EQ(r.selEndLine, 20);
    ASSERT_EQ(r.selEndColumn, 14);
}
