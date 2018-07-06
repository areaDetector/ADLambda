#include "MemUtilsTest.h"

namespace TestNS
{
    void MemUtilsTest::SetUp()
    {
        m_buffer_size = 50;
        m_element_size = 100;
        m_written_imgs = 50;
        m_safty_margin = 32;
        
        m_mempool_int16 = unique_ptr<MemPool<int16>>(
            new MemPool<int16>(m_buffer_size,m_element_size));

        m_img = shared_ptr<int16>(new int16[m_element_size]);
    }

    void MemUtilsTest::TearDown()
    {
        m_mempool_int16.reset(nullptr);
    }

    void MemUtilsTest::GenerateRandom(szt img_numbers)
    {
        m_written_imgs = img_numbers;
        m_frame_no.resize(m_written_imgs);
        m_error_code.resize(m_written_imgs);

        set<int32> frame_no;
        while(frame_no.size() < m_written_imgs)
            frame_no.insert((std::rand() % m_written_imgs));

        m_frame_no.assign(frame_no.begin(),frame_no.end());
        
        // std::generate(m_frame_no.begin(),m_frame_no.end(),
        //               [&](){return std::rand() % m_written_imgs;});
        std::generate(m_error_code.begin(),m_error_code.end(),
                      [&](){return std::rand() % 3;});   
    }

    void MemUtilsTest::WriteImages(szt img_numbers)
    {
        GenerateRandom(img_numbers);
        for(szt i=0; i<img_numbers; i++)
            m_mempool_int16->SetImage(m_img.get(),m_frame_no[i],m_error_code[i]);
    }

    void MemUtilsTest::WriteImagesInOrder(szt img_numbers)
    {
        GenerateRandom(img_numbers);
        for(szt i=0; i<img_numbers; i++)
            m_mempool_int16->SetImage(m_img.get(),m_frame_no[i],m_error_code[i],true);
    }
    
    TEST_F(MemUtilsTest,TestMemPoolInit)
    {
        EXPECT_EQ(m_mempool_int16->GetStoredImageNumbers(),0);
        EXPECT_EQ(m_mempool_int16->GetFirstFrameNo(),-1);
        EXPECT_EQ(m_mempool_int16->GetTotalReceivedFrames(),0);
        EXPECT_EQ(m_mempool_int16->GetLastArrivedImageNo(),0);
        EXPECT_EQ(m_mempool_int16->IsImageReadyForReading(1),false);
        EXPECT_EQ(m_mempool_int16->GetStartPosOfBuffer(),0);
        EXPECT_EQ(m_mempool_int16->GetTotalReceivedPackets(),0);
        EXPECT_EQ(m_mempool_int16->GetFrameNoByIndex(1),int32min);
        EXPECT_EQ(m_mempool_int16->GetElementSize(),m_element_size);
        EXPECT_EQ(m_mempool_int16->IsFull(),false);
    }

    TEST_F(MemUtilsTest,TestWriteImages)
    {
        WriteImages(m_written_imgs);

        EXPECT_EQ(static_cast<szt>(m_mempool_int16->GetStoredImageNumbers()),
                  (m_written_imgs - m_safty_margin));

        if(m_written_imgs == m_buffer_size)
            EXPECT_EQ(m_mempool_int16->IsFull(),true);
    }

    TEST_F(MemUtilsTest,TestWriteImagesInOrder)
    {
        WriteImagesInOrder(m_written_imgs);

        EXPECT_EQ(static_cast<szt>(m_mempool_int16->GetStoredImageNumbers()),
                  (m_written_imgs - m_safty_margin));

        if(m_written_imgs == m_buffer_size)
            EXPECT_EQ(m_mempool_int16->IsFull(),true);
    }
    
    TEST_F(MemUtilsTest,DISABLED_TestGetImages)
    {
        WriteImages(m_written_imgs);

        int32 data_size;
        vector<int32> frame_no_get;
        vector<int16> error_code_get;
        int16* img_get;
        
        for(szt i=0; i<m_written_imgs; i++)
        {
            int32 frame_no;
            int16 error_code;
            EXPECT_EQ(m_mempool_int16->GetImage(img_get,frame_no,error_code,data_size),true);
            EXPECT_EQ(data_size,0);
            frame_no_get.push_back(frame_no);
            error_code_get.push_back(error_code);
        }

        EXPECT_EQ(m_frame_no,frame_no_get);
        EXPECT_EQ(m_error_code,error_code_get);
    }

    TEST_F(MemUtilsTest,DISABLED_TestGetImagesWithFixedPos)
    {
        WriteImagesInOrder(m_written_imgs);
        
        int32 data_size;
        vector<int32> frame_no_get;
        vector<int16> error_code_get;
        int16* img_get;
        
        for(szt i=0; i<m_written_imgs; i++)
        {
            int32 frame_no;
            int16 error_code;
            EXPECT_EQ(m_mempool_int16->GetImage(img_get,frame_no,error_code,data_size),true);
            EXPECT_EQ(data_size,0);
            frame_no_get.push_back(frame_no);
            error_code_get.push_back(error_code);
        }

        //order the data from ascend
        std::sort(m_frame_no.begin(),m_frame_no.end(),std::less<int32>());
        //std::sort(m_error_code.begin(),m_error_code.end(),std::less<int16>());
        
        EXPECT_EQ(m_frame_no,frame_no_get);
        //EXPECT_EQ(m_error_code,error_code_get);
    }
}
