//
//  typedef-tests.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#include "typedef.hpp"
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
// Line 48: "    return color;"
const unsigned line = 48;
const unsigned column = 14;

class TypeDefinitionTest : public ::testing::Test
{
protected:
    std::shared_ptr<ITypeDefinition> typeDefinition;
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
        store->SaveHeaders(); // without opencl-c.h, the 'GetTypeDefinitions' cascades into broken cursor resolution
        store->SetTranslationOptions(options);
        typeDefinition = CreateTypeDefinition(store);
        
        store->OnFileOpen(KERNEL_FILE, fileContent);
    }
    
    void TearDown() override
    {
        store->OnFileClose(KERNEL_FILE);
    }
};

} // namespace

// Test basic definition at a function call site
TEST_F(TypeDefinitionTest, TypeDefinitionAtFunctionCall)
{
    auto results = typeDefinition->GetTypeDefinitions(KERNEL_FILE, line, column);
    ASSERT_GT(results.size(), 0);

    auto r = results.front();
    ASSERT_EQ(r.uri, KERNEL_FILE);
    ASSERT_EQ(r.startLine, 5);
    ASSERT_EQ(r.startColumn, 8);
    ASSERT_EQ(r.endLine, 5);
    ASSERT_EQ(r.endColumn, 33);
    ASSERT_EQ(r.selStartLine, 5);
    ASSERT_EQ(r.selStartColumn, 8);
    ASSERT_EQ(r.selEndLine, 5);
    ASSERT_EQ(r.selEndColumn, 13);
}
