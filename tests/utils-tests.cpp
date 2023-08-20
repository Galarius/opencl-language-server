//
//  utils-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 20/8/23.
//

#include "utils.hpp"

#include <gtest/gtest.h>


using namespace ocls;


// --- SplitString ---


TEST(UtilsTest, BasicSplit)
{
    std::string str = "apple,banana,orange";
    auto result = utils::SplitString(str, ",");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "apple");
    EXPECT_EQ(result[1], "banana");
    EXPECT_EQ(result[2], "orange");
}

TEST(UtilsTest, NoSplitPattern)
{
    std::string str = "applebananaorange";
    auto result = utils::SplitString(str, ",");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "applebananaorange");
}

TEST(UtilsTest, EmptyString)
{
    std::string str = "";
    auto result = utils::SplitString(str, ",");
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].empty());
}

TEST(UtilsTest, EmptyPattern)
{
    std::string str = "applebananaorange";
    auto result = utils::SplitString(str, "");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "applebananaorange");
}

TEST(UtilsTest, SplitAtEveryCharacter)
{
    std::string str = "apple";
    auto result = utils::SplitString(str, "p");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "");
    EXPECT_EQ(result[2], "le");
}

// --- UriToFilePath ---

TEST(UriToFilePathTest, BasicUri)
{
    std::string uri = "file:///C:/folder/file.txt";
    auto result = utils::UriToFilePath(uri, false);
    EXPECT_EQ(result, "C:\\folder\\file.txt");
}

TEST(UriToFilePathTest, UriWithSpaces)
{
    std::string uri = "file:///C:/some%20folder/file%20name.txt";
    auto result = utils::UriToFilePath(uri, false);
    EXPECT_EQ(result, "C:\\some folder\\file name.txt");
}

TEST(UriToFilePathTest, UnixPath)
{
    std::string uri = "file:///home/user/folder/file.txt";
    auto result = utils::UriToFilePath(uri, true);
    EXPECT_EQ(result, "/home/user/folder/file.txt");
}

TEST(UriToFilePathTest, EmptyUri)
{
    std::string uri = "";
    auto result = utils::UriToFilePath(uri, true);
    EXPECT_EQ(result, ""); // Assuming it returns empty string
}
