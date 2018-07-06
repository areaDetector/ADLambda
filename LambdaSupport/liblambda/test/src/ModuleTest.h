#pragma once


#include "LambdaConfigReader.h"
#include "LambdaModule.h"

#include "MockNetwork.h"
#include <fsdetector/core/FilesOperation.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
namespace TestNS
{
    using namespace FSDetCoreNS;
    using namespace DetLambdaNS;
    
    using namespace std;

    using ::testing::SetArgPointee;
    using ::testing::SetArgReferee;
    using ::testing::SaveArg;
    using ::testing::AtLeast;
    using ::testing::Return;
    using ::testing::DoAll;
    using ::testing::_;
    

    typedef shared_ptr<MockNetwork> sptr_network;
    typedef unique_ptr<FileOperation> uptr_file;
    
    typedef unique_ptr<LambdaConfigReader> uptr_config;
    typedef unique_ptr<LambdaModule> uptr_module;
    
    
    class ModuleTest : public testing::Test
    {
      public:
        void OutputData(vector<uchar> data);
        void OutputData1(char* data,szt length);
        
      protected:
        virtual void SetUp();
        virtual void TearDown();

        void ReadConfig(bool switch_mode,string opmode);
        void PrintVector(vector<uchar>& data);

        sptr_network m_network;
        uptr_config m_config;
        uptr_file m_file;
        uptr_module m_module;

        int16 m_tcpport;
        bool m_multilink;
        
        string m_filepath,m_moduleid,m_tcpip,m_opmode;

        vector<uint16> m_portlist;
        vector<int16> m_currentchips;
        vector<char> m_cmd;
        vector<uchar> m_outputdata;
        vector<vector<string>> m_maclist,m_iplist;
        vector<stMedipixChipData> m_chipdata;

        stDetCfgData m_configdata;
    };
}
