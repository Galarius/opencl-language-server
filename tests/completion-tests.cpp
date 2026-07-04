//
//  completion-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 3/6/26.
//

#include "completion.hpp"
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
// Line 62: "            p = get"
// Position 20 is after "get" to trigger completion for getChannel
const unsigned line = 62;
const unsigned column = 20;

class CompletionTest : public ::testing::Test
{
protected:
    std::shared_ptr<ICompletion> completion;
    std::shared_ptr<ITranslationUnitStore> store;

    void SetUp() override
    {
        std::string fileContent;
        std::ifstream f(KERNEL_FILE);
        if (f.is_open())
        {
            fileContent.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        }
        store = CreateTranslationUnitStore();
        store->SetTranslationOptions({});
        store->OnFileOpen(KERNEL_FILE, fileContent);
        completion = CreateCompletion(store);
    }
    
    void TearDown() override
    {
        store->OnFileClose(KERNEL_FILE);
    }
};

} // namespace

// Test basic completion at a function call site
TEST_F(CompletionTest, CompletionAtFunctionCall)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    ASSERT_GT(results.size(), 0);

    // Find getChannel in results
    auto it =
        std::find_if(results.begin(), results.end(), [](const CompletionResult& r) { return r.label == "getChannel"; });
    ASSERT_NE(it, results.end());

    // Verify completion result properties
    EXPECT_EQ(it->itemKind, CompletionItemKind::function);
    EXPECT_FALSE(it->insertText.empty());
    EXPECT_FALSE(it->deprecated);
    EXPECT_GT(it->sortText.length(), 0);
}

// Test that results include proper sort order
TEST_F(CompletionTest, CompletionSortOrder)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    ASSERT_GT(results.size(), 0);

    // Verify each result has valid numeric sort text
    for (const auto& result : results)
    {
        EXPECT_GT(result.sortText.length(), 0);
        EXPECT_TRUE(std::all_of(result.sortText.begin(), result.sortText.end(), [](char c) { return std::isdigit(c); }))
            << "sortText should be all digits: " << result.sortText;
    }
}

// Test completion item kinds are properly mapped
TEST_F(CompletionTest, CompletionItemKinds)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    ASSERT_GT(results.size(), 0);

    // Check that we have at least one function
    auto hasFunctions = std::any_of(results.begin(), results.end(), [](const CompletionResult& r) {
        return r.itemKind == CompletionItemKind::function;
    });
    EXPECT_TRUE(hasFunctions);

    // All items should have a valid kind
    for (const auto& result : results)
    {
        EXPECT_GE(static_cast<unsigned>(result.itemKind), 0);
        EXPECT_LE(static_cast<unsigned>(result.itemKind), 25);
    }
}

// Test that function completions have commit characters
TEST_F(CompletionTest, FunctionCommitCharacters)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    auto it =
        std::find_if(results.begin(), results.end(), [](const CompletionResult& r) { return r.label == "getChannel"; });
    ASSERT_NE(it, results.end());

    // Functions should have commit characters
    ASSERT_GT(it->commitCharacters.size(), 0);
    EXPECT_EQ(it->commitCharacters[0], "(");
}

// Test completion has all required fields
TEST_F(CompletionTest, CompletionResultFields)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    ASSERT_GT(results.size(), 0);

    for (const auto& result : results)
    {
        EXPECT_FALSE(result.label.empty());
        EXPECT_GT(result.sortText.length(), 0);
        EXPECT_FALSE(result.insertText.empty());
        // detail and documentation may be empty for some items
        EXPECT_GE(static_cast<unsigned>(result.itemKind), 0);
    }
}

// Test tags for deprecated items
TEST_F(CompletionTest, DeprecatedTags)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    for (const auto& result : results)
    {
        if (result.deprecated)
        {
            // If deprecated, should have tag 1 (Deprecated)
            ASSERT_GT(result.tags.size(), 0);
            EXPECT_EQ(result.tags[0], 1); // CompletionItemTag::Deprecated
        }
    }
}

// Test that results have detail information for functions
TEST_F(CompletionTest, FunctionDetail)
{
    auto results = completion->GetCompletions(KERNEL_FILE, line, column);

    auto it =
        std::find_if(results.begin(), results.end(), [](const CompletionResult& r) { return r.label == "getChannel"; });

    if (it != results.end() && it->itemKind == CompletionItemKind::function)
    {
        // Detail should contain parameter info or return type
        // (may be empty if not available from clang)
        EXPECT_TRUE(!it->detail.empty() || it->insertText.find("()") != std::string::npos);
    }
}
