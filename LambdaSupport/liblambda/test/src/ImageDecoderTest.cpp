#include "ImageDecoderTest.h"
#include "TestUtils.h"

namespace TestNS
{
    void ImageDecoderTest::SetUp()
    {
        vector<int16> current_chips = {1,2,3,4,5,6,7,8,9,10,11,12};
        m_x = 1536;
        m_y = 512;
        m_bytesofpixel = 2;

        m_size = m_x*m_y;
        m_sizeinbytes = m_size * m_bytesofpixel;

        m_raw.resize(m_sizeinbytes,0);
        m_decoded.resize(m_size,0);
        m_raw_img_path = string("./testdata/raw_12chip_image.dat");
        m_decoded_image_path = string("./testdata/decoded_12chip_image.dat");
        m_decoder = uptr_decoder(new ImageDecoder(current_chips));
    }
    
    void ImageDecoderTest::TearDown()
    {
        m_decoder.reset(nullptr);
    }

    void ImageDecoderTest::ReadData(vector<char>& data,string path,int32 bytes)
    {
        ifstream ifs(path,ios::in|ios::binary);
        
        char pixel;
  
        int32 i=0;
        while(!ifs.eof())
        {
            ifs.read((char*)(&pixel),bytes);
            if(!ifs.eof())
            {  
                data[i] = pixel;
                i++;
            }
        }
        ifs.close();
    }

    void ImageDecoderTest::ReadData(vector<int16>& data,string path,int32 bytes)
    {        
        ifstream ifs(path,ios::in|ios::binary);
        
        int16 pixel;
  
        int32 i=0;
        while(!ifs.eof())
        {
            ifs.read((char*)(&pixel),bytes);
            if(!ifs.eof())
            {
                data[i] = pixel;
                i++;
            }
        }
        ifs.close();
    }
    
    void ImageDecoderTest::WriteData(vector<int16>& data)
    {
        ofstream ofs(m_decoded_image_path,ios::out|ios::binary);
        for(auto val:data)
            ofs.write((char*)&val,sizeof(int16));
        ofs.close();
    }
    
    TEST_F(ImageDecoderTest,TestImageDecoder)
    {   
        //get raw image
        ReadData(m_raw,m_raw_img_path,sizeof(char));

        //get reference decoded image
        ReadData(m_decoded,m_decoded_image_path,sizeof(int16));

        //do decoding
        char* raw_data = reinterpret_cast<char*>(&m_raw[0]);
        m_decoder->SetRawImage(raw_data);

        TimeMeasurement time;
        time.Start();
        
        int16* decoded_data = m_decoder->RunDecodingImg();

        time.Stop();

        string msg = "The image decoding takes:"
            + to_string(time.GetDifferenceInMicroSecond()/1000)
            +" ms";

        std::cout << "[          ] " << msg.c_str() << std::endl;
        
        vector<int16> dst_data(decoded_data,decoded_data+m_size);
        
        EXPECT_EQ(m_decoded,dst_data); 
    }
}


