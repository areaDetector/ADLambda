#pragma once

#include "ImageDecoder.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
namespace TestNS
{
    using namespace FSDetCoreNS;
    using namespace DetLambdaNS;
    
    using namespace std;

    using ::testing::SaveArg;
    using ::testing::AtLeast;
    using ::testing::Return;
    using ::testing::_;
    

    typedef unique_ptr<ImageDecoder> uptr_decoder;
    
    class ImageDecoderTest : public testing::Test
    {
    protected:
        virtual void SetUp();
        virtual void TearDown();

        //binary data
        void ReadData(vector<char>& data,string path,int32 bytes);
        void ReadData(vector<int16>& data,string path,int32 bytes);
        void WriteData(vector<int16>& data);
    };

    uptr_decoder m_decoder;
    uint32 m_x,m_y,m_bytesofpixel,m_size,m_sizeinbytes;
    vector<char> m_raw;
    vector<int16> m_decoded;

    string m_raw_img_path,m_decoded_image_path;
}
