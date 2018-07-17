#include "DistortionCorrectorTest.h"
#include "TestUtils.h"

namespace TestNS
{
    void DistortionCorrectorTest::SetUp()
    {
        string config_path = string("./testdata");
        m_config = uptr_config(new LambdaConfigReader(config_path));
        m_config->LoadLocalConfig(false,"");

        m_index = m_config->GetIndexFile();
        m_nominator = m_config->GetNominatorFile();
        m_config->GetDistoredImageSize(m_x,m_y);
        m_size = m_x*m_y;

        m_saturatedpixel = static_cast<int32>(pow(2,12)-1);
        m_dc16 = uptr_dc16(new DistortionCorrector<int16>(m_index,m_nominator,m_saturatedpixel));
    }

    void DistortionCorrectorTest::TearDown()
    {
        m_config.reset(nullptr);
        m_dc16.reset(nullptr);
    }

    void DistortionCorrectorTest::ReadData(vector<int16>& data,string path,int32 bytes)
    {
        ifstream ifs(path,ios::in|ios::binary);

        int16 pixel;

        int32 i=0;
        while(!ifs.eof())
        {
            ifs.read((char*)(&pixel),bytes);
            if(!ifs.eof())
            {
                data.push_back(pixel);
                i++;
            }
        }
        ifs.close();
    }

    void DistortionCorrectorTest::WriteData(vector<int16>& data,string path)
    {
        ofstream ofs(path,ios::out|ios::binary);
        for(auto val:data)
            ofs.write((char*)&val,sizeof(int16));
        ofs.close();
    }

    TEST_F(DistortionCorrectorTest,TestRunDistortCorrect)
    {
        //read decoded image
        string decoded_image_path = string("./testdata/decoded_12chip_image.dat");
        ReadData(m_decoded_img,decoded_image_path,2);
        EXPECT_EQ(m_decoded_img.size(),static_cast<szt>(1536*512));
        int16* decoded_img = reinterpret_cast<int16*>(&m_decoded_img[0]);

        //read reference distorted image
        string distorted_image_path = string("./testdata/distorted_12chip_image.dat");
        ReadData(m_distorted_img,distorted_image_path,2);
        EXPECT_EQ(m_distorted_img.size(),static_cast<szt>(m_size));

        TimeMeasurement time;
        time.Start();

        //do distortion correction
        int16* distored_img = m_dc16->RunDistortCorrect(decoded_img);

        time.Stop();

        string msg = "The image decoding takes:"
            + to_string(time.GetDifferenceInMicroSecond()/1000)
            +" ms\n";

        //testing::internal::PRINTMSG(msg);

        vector<int16> dst_img(distored_img,distored_img+m_size);
        EXPECT_EQ(m_distorted_img,dst_img);
        // WriteData(dst_img,distorted_image_path);
    }

}
