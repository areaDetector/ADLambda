#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Globals.h"
#include "FilesOperation.h"


namespace TestNS
{
    using namespace FSDetCoreNS;
    using ::testing::AtLeast;
    using ::testing::Return;

    class FilesOperationTest : public testing::Test
    {
      protected:
        virtual void SetUp();
        virtual void TearDown();
        
        string m_filename_txt_exist,
            m_filename_bin_exist,
            m_filename_non_exist;

        unique_ptr<FileOperation> m_filereader;
        unique_ptr<FileOperation> m_filewriter;
    };
}
