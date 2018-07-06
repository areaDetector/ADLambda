#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Globals.h"
#include "Utils.h"

namespace TestNS
{
    using namespace FSDetCoreNS;
    using ::testing::AtLeast;
    using ::testing::Return;

    class UtilsTest : public testing::Test
    {
      protected:
        virtual void SetUp();
        virtual void TearDown();

        string m_original_string;
        
        unique_ptr<StringUtils> m_strutils;
        unique_ptr<FileUtils> m_fileutils;
        unique_ptr<DataConvert> m_dataconvert;
        
    };
}
