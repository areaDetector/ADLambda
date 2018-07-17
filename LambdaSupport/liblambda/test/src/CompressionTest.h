#pragma once

#include "CompressionContext.h"

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


    typedef unique_ptr<CompressionContext> uptr_compressioncxt;
    
    class CompressionTest : public testing::Test
    {
    protected:
        virtual void SetUp();
        virtual void TearDown();

        vector<uchar>& GenerateRandomList(szt data_size,uint32 element_bytes,uint32 range);
        vector<uchar>& GenerateSequenceList(szt data_size,uint32 element_bytes);

        uptr_compressioncxt m_cxt;
        
        uint32 m_range,m_bytesofelement;
        szt m_datasize;
        
        vector<uchar> m_src;
    };  
}
