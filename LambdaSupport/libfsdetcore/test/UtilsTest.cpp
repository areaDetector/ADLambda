#include "UtilsTest.h"

namespace TestNS
{
    void UtilsTest::SetUp()
    {
        m_original_string = string("this is a test string needs to be splitted.");
        m_strutils = unique_ptr<StringUtils>(new StringUtils());
        m_fileutils = unique_ptr<FileUtils>(new FileUtils());
        m_dataconvert = unique_ptr<DataConvert>(new DataConvert());
    }

    void UtilsTest::TearDown()
    {
        m_strutils.reset(nullptr);
        m_fileutils.reset(nullptr);
        m_dataconvert.reset(nullptr);
    }

    TEST_F(UtilsTest,TestStrSplit)
    {
        string delimiter(" ");
        vector<string> expected_string_list={"this","is","a","test","string",
                                             "needs","to","be","splitted."};
        vector<string> result_string;
        m_strutils->StrSplit(m_original_string,delimiter,result_string);
        EXPECT_EQ(expected_string_list,result_string);
    }

    TEST_F(UtilsTest,TestStrSplitNoMatch)
    {
        string delimiter (".");
        string expected_string = m_original_string.substr(0,(strlen(m_original_string.c_str())-1));
        vector<string> expected_string_list = {expected_string};
        vector<string> result_string;
        m_strutils->StrSplit(m_original_string,delimiter,result_string);
        EXPECT_EQ(expected_string_list,result_string);
    }

    TEST_F(UtilsTest,TestFindAndReplace)
    {
        string to_be_replaced ("is");
        string replace ("hhh");
        string expected_result ("thhhh hhh a test string needs to be splitted.");
        string result_string = m_original_string;
        m_strutils->FindAndReplace(result_string,to_be_replaced,replace);
        EXPECT_EQ(expected_result,result_string);
    }

    TEST_F(UtilsTest,TestFindAndReplaceNoMatch)
    {
        string to_be_replaced ("abcde");
        string replace ("hhh");
        string expected_result = m_original_string;
        string result_string = m_original_string;
        m_strutils->FindAndReplace(result_string,to_be_replaced,replace);
        EXPECT_EQ(expected_result,result_string);
    }

    TEST_F(UtilsTest,TestRemoveChar)
    {
        char delimiter = 's';
        string expected_result ("thi i a tet tring need to be plitted.");
        m_strutils->RemoveChar(m_original_string,delimiter);
        EXPECT_EQ(expected_result,m_original_string);
    }

    TEST_F(UtilsTest,TestRemoveCharNoMatch)
    {
        char delimiter = 'z';
        string expected_result = m_original_string;
        m_strutils->RemoveChar(m_original_string,delimiter);
        EXPECT_EQ(expected_result,m_original_string);
    }

    TEST_F(UtilsTest,TestReverseChar)
    {
        char src = 0xf0;
        char expected = 0x0f;
        char result = StringUtils::ReverseChar(src);
        EXPECT_EQ(expected,result);
    }

    TEST_F(UtilsTest,TestGetFileList)
    {
        string file_path ("./testdata");
        vector<string> expected_result = {"test.txt","test.bin"};
        vector<string> result = m_fileutils->GetFileList(file_path);
        EXPECT_EQ(expected_result,result);
    }

    TEST_F(UtilsTest,TestGetFileListNonExistPath)
    {
        string file_path ("./pathnonexist");
        vector<string> result = m_fileutils->GetFileList(file_path);
        EXPECT_EQ(result.size(),static_cast<uint16>(0));
    }

    TEST_F(UtilsTest,TestIntToUChar)
    {
        int test = 0x0a0b0c0d;
        vector<uint8> expected_result = {0x0a,0x0b,0x0c,0x0d};
        vector<uint8> result(4,0x0);
        m_dataconvert->IntToUChar(test,result);
        EXPECT_EQ(expected_result,result);
    }
}
