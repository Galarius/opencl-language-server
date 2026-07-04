//
//  translation-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "translation.hpp"

#include <gmock/gmock.h>

class TranslationUnitStoreMock : public ocls::ITranslationUnitStore
{
public:
    MOCK_METHOD(void, OnFileOpen, (const std::string &, const std::string &), (override));
    MOCK_METHOD(void, OnFileChange, (const std::string &, const std::string &), (override));
    MOCK_METHOD(void, OnFileClose, (const std::string &), (override));
    MOCK_METHOD(void, SetTranslationOptions, (const std::vector<std::string> &), (override));
    MOCK_METHOD(void, SaveHeaders, (), (override));

    MOCK_METHOD(CXTranslationUnit, GetTranslationUnit, (const std::string &), (const override));
    MOCK_METHOD(std::string *, GetContent, (const std::string &), (const override));
    MOCK_METHOD(CXCursor, GetCursorAt, (const std::string &, unsigned, unsigned), (const override));
};
