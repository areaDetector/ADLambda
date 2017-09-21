#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Globals.h"
#include "MemUtils.h"

namespace TestNS
{
    using namespace FSDetCoreNS;
    using ::testing::AtLeast;
    using ::testing::Return;

    class MemUtilsTest : public testing::Test
    {
      protected:
        virtual void SetUp();
        virtual void TearDown();

        void GenerateRandom(szt img_numbers);
        void WriteImages(szt img_numbers);
        void WriteImagesInOrder(szt img_numbers);
        
        szt m_element_size,m_buffer_size,m_written_imgs,m_safty_margin;
        vector<int32> m_frame_no;
        vector<int16> m_error_code;
        unique_ptr<MemPool<int16>> m_mempool_int16;
        shared_ptr<int16> m_img;
    };
}
