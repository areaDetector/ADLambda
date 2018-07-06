#include "CompressionTest.h"
#include "Compression.h"

namespace TestNS
{
    void CompressionTest::SetUp()
    {
     
        m_cxt = uptr_compressioncxt(
            new CompressionContext(
                unique_ptr<CompressionInterface>(new CompressionZlib())));

        m_datasize = 1556*512;
        m_bytesofelement = sizeof(uint32);
        m_range = uint32max;
        m_src.resize(m_datasize*m_bytesofelement,0);
    }
    
    void CompressionTest::TearDown()
    {
        m_cxt.reset(nullptr);
        m_src.clear();
    }

    vector<uchar>& CompressionTest::GenerateRandomList(szt data_size,
                                                       uint32 element_bytes,
                                                       uint32 range)
    {
        vector<uint32> src;
        for(szt i=0; i<data_size; i++)
            src.push_back((rand() % range));

        memmove(&m_src[0],&src[0],data_size*element_bytes);
        return m_src;
        
    }

    vector<uchar>& CompressionTest::GenerateSequenceList(szt data_size,
                                                         uint32 element_bytes)
    {
        vector<uint32> src;
        for(szt i=0; i<data_size; i++)
            src.push_back(i+1);

        memmove(&m_src[0],&src[0],data_size*element_bytes);
        return m_src;
    }

    TEST_F(CompressionTest,TestCompressionWithInvalidLevel)
    {
        vector<uchar> dst;
        
        //level 10, should return false
        EXPECT_EQ(m_cxt->CompressData(m_src,dst,10),false);
    }
    
    TEST_F(CompressionTest, TestCompressionWithRandomData)
    {
        vector<uchar> src = GenerateRandomList(m_datasize,m_bytesofelement,m_range);
        vector<uchar> dst;
        for(szt i=1; i<=9; i++)
        {
            EXPECT_EQ(m_cxt->CompressData(src,dst,i),true);
        }
    }

    TEST_F(CompressionTest,TestCompressionWithSequenceData)
    {
        vector<uchar> src = GenerateSequenceList(m_datasize,m_bytesofelement);
        vector<uchar> dst;
        for(szt i=1; i<=9; i++)
        {
            EXPECT_EQ(m_cxt->CompressData(src,dst,i),true);
        }
    }
}


