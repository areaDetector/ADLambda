#pragma once


#include "DistortionCorrector.h"
#include "LambdaConfigReader.h"

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

    typedef unique_ptr<DistortionCorrector<int16>> uptr_dc16;
    typedef unique_ptr<LambdaConfigReader> uptr_config;
    
    class DistortionCorrectorTest : public testing::Test
    {
    protected:
        virtual void SetUp();
        virtual void TearDown();

        void ReadData(vector<int16>& data,string path,int32 bytes);
        void WriteData(vector<int16>& data,string path);
        

        uptr_dc16 m_dc16;
        uptr_config m_config;

        int32 m_saturatedpixel,m_x,m_y,m_size;
        
        vector<int32> m_index,m_nominator;
        vector<int16> m_decoded_img,m_distorted_img;
    };    
}
