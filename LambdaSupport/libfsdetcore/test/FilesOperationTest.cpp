#include "FilesOperationTest.h"

namespace TestNS
{
    void FilesOperationTest::SetUp()
    {
        m_filename_bin_exist = string("./testdata/test.bin");
        m_filename_txt_exist = string("./testdata/test.txt");
        m_filename_non_exist = string("./testdata/nonexistfile");
        m_filereader = unique_ptr<FileOperation>(new FileReader());
        //m_filewriter = unique_ptr<FileOperation>(new FileWriter());
    }

    void FilesOperationTest::TearDown()
    {
        m_filereader.reset(nullptr);
        //m_filewriter.reset(nullptr);
    }

    TEST_F(FilesOperationTest,TestOpenTextFile)
    {
        m_filereader->SetFilePath(m_filename_txt_exist);
        EXPECT_TRUE(m_filereader->OpenFile(false));
        m_filereader->CloseFile();
    }
    
    TEST_F(FilesOperationTest,TestOpenBinFile)
    {
        m_filereader->SetFilePath(m_filename_bin_exist);
        EXPECT_TRUE(m_filereader->OpenFile(true));
        m_filereader->CloseFile();
    }

    TEST_F(FilesOperationTest,TestOpenNonExistFile)
    {
        m_filereader->SetFilePath(m_filename_non_exist);
        EXPECT_FALSE(m_filereader->OpenFile(false));
        m_filereader->CloseFile();
    }
    
    TEST_F(FilesOperationTest,TestReadTextFile)
    {
        vector<string> expected_content_list = {
            string("defineOperationMode TwentyFourBit 0 3 Twenty_four_bit_readout"),
            string("defineSrcIPAddressCH1 196.254.3.41"),
            string("defineDetPortTCP")};

        m_filereader->SetFilePath(m_filename_txt_exist);
        EXPECT_TRUE(m_filereader->OpenFile(false));
        vector<string> src_content_list = m_filereader->ReadDataFromFile();
        EXPECT_EQ(src_content_list,expected_content_list);
        m_filereader->CloseFile();
    }
    
    TEST_F(FilesOperationTest,TestReadBinaryFileShort)
    {
        vector<int16> expected_value_list = {0x4800,0x4900};
        m_filereader->SetFilePath(m_filename_bin_exist);
        EXPECT_TRUE(m_filereader->OpenFile(true));
        vector<int16> src_value_list = m_filereader->ReadDataFromBinaryFile();
        EXPECT_EQ(src_value_list,expected_value_list);
        m_filereader->CloseFile();
    }

    TEST_F(FilesOperationTest,TestReadBinaryFileInt)
    {
        vector<int32> expected_value_list = {0x490048};
        m_filereader->SetFilePath(m_filename_bin_exist);
        EXPECT_TRUE(m_filereader->OpenFile(true));
        vector<int32> src_value_list = m_filereader->ReadDataFromIntBinaryFile();
        EXPECT_EQ(src_value_list,expected_value_list);
        m_filereader->CloseFile();
    }

    
}
