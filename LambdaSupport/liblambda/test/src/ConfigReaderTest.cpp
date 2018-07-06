
#include "ConfigReaderTest.h"

namespace TestNS
{
    void ConfigReaderTest::SetUp()
    {
        string path = string("./testdata");
        m_configreader = uptr_config(new LambdaConfigReader(path));
    }
    
    void ConfigReaderTest::TearDown()
    {
        m_configreader.reset(nullptr);
    }

    TEST_F(ConfigReaderTest,TestInit)
    {
        vector<vector<string>> maclist,iplist;
        vector<uint16> portlist;
        m_configreader->GetUDPConfig(maclist,iplist,portlist);

        vector<vector<string>> ref_maclist(2,vector<string>(2,"00:00:00:00:00:00"));
        vector<vector<string>> ref_iplist(2,vector<string>(2,"127.0.0.1"));
        vector<uint16> ref_portlist{0,UDP_PORT,UDP_PORT_1};
        ref_iplist[0][0] = UDP_CONTROL_IP_ADDRESS;
        ref_iplist[1][0] = UDP_CONTROL_IP_ADDRESS_1;
        
        EXPECT_EQ(maclist,ref_maclist);
        EXPECT_EQ(iplist,ref_iplist);
        EXPECT_EQ(portlist,ref_portlist);
        
        string tcpip;
        int16 tcpport;
        m_configreader->GetTCPConfig(tcpip,tcpport);
        EXPECT_EQ(TCP_CONTROL_IP_ADDRESS,tcpip);
        EXPECT_EQ(TCP_CONTROL_PORT,tcpport);
        
        EXPECT_EQ(m_configreader->GetMultilink(),MULTI_LINK);

        EXPECT_EQ(m_configreader->GetBurstMode(),true);

        vector<int16> current_chips;
        vector<stMedipixChipData> chip_data;
        m_configreader->GetChipConfig(current_chips,chip_data);
        m_configreader->GetOperationMode();
        m_configreader->GetModuleName();

        stDetCfgData config_data = m_configreader->GetDetConfigData();

        szt expected_size = 0;
        EXPECT_EQ((m_configreader->GetPixelMask()).size(),expected_size);
        EXPECT_EQ((m_configreader->GetIndexFile()).size(),expected_size);
        EXPECT_EQ((m_configreader->GetNominatorFile()).size(),expected_size);

        vector<float> ref_position(3,-1.0);
        EXPECT_EQ(m_configreader->GetPosition(),ref_position);

        int32 x,y;
        m_configreader->GetDistoredImageSize(x,y);
        EXPECT_EQ(x,-1);
        EXPECT_EQ(y,-1);
        
        EXPECT_EQ(m_configreader->GetRawBufferLength(),RAW_BUFFER_LENGTH);
        EXPECT_EQ(m_configreader->GetDecodedBufferLength(),DECODED_BUFFER_LENGTH);
        
        vector<int32> ref_threadnumber={3,3,6};
        EXPECT_EQ(m_configreader->GetDecodingThreadNumbers(),ref_threadnumber);
        EXPECT_EQ(m_configreader->GetCriticalShutterTime(),2.1);
        EXPECT_EQ(m_configreader->GetSlaveModule(),false);
    }

    TEST_F(ConfigReaderTest,TestLoadLocalConfigFirstRun)
    {
        m_configreader->LoadLocalConfig(false,"");
        
        vector<vector<string>> maclist,iplist;
        vector<uint16> portlist;
        m_configreader->GetUDPConfig(maclist,iplist,portlist);

        vector<vector<string>> ref_maclist
            = { { "90:B1:1C:3A:2C:D7", "00:0c:f1:f4:b0:4d" },
                { "B8:CA:3A:EE:6B:A7", "00:0c:f1:f4:b0:4d" } };

        vector<vector<string>> ref_iplist
            = { { "196.254.1.4", "196.254.1.41" },
                { "196.254.3.7", "196.254.3.41" } };

        vector<uint16> ref_portlist = { 4320, 4321, 4422 };
        
        EXPECT_EQ(maclist,ref_maclist);
        EXPECT_EQ(iplist,ref_iplist);
        EXPECT_EQ(portlist,ref_portlist);
        
        string tcpip;
        int16 tcpport;
        m_configreader->GetTCPConfig(tcpip,tcpport);

        string ref_tcpip = string("169.254.1.1");
        int16 ref_tcpport = 4321;
        EXPECT_EQ(ref_tcpip,tcpip);
        EXPECT_EQ(ref_tcpport,tcpport);
        
        EXPECT_EQ(m_configreader->GetMultilink(),true);

        EXPECT_EQ(m_configreader->GetBurstMode(),true);

        vector<int16> current_chips;
        vector<stMedipixChipData> chip_data;
        m_configreader->GetChipConfig(current_chips,chip_data);
        EXPECT_EQ(m_configreader->GetOperationMode(),"ContinuousReadWrite");
        EXPECT_EQ(m_configreader->GetModuleName(),"FullRXModule7");

        stDetCfgData config_data = m_configreader->GetDetConfigData();
        int32 ref_x = 1556;
        int32 ref_y = 516;
        szt ref_size = ref_x*ref_y;
        
        EXPECT_EQ((m_configreader->GetPixelMask()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetIndexFile()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetNominatorFile()).size(),ref_size);

        vector<float> ref_position(3,-1.0);
        EXPECT_EQ(m_configreader->GetPosition(),ref_position);

        int32 x,y;
        m_configreader->GetDistoredImageSize(x,y);
        EXPECT_EQ(x,1556);
        EXPECT_EQ(y,516);
        
        EXPECT_EQ(m_configreader->GetRawBufferLength(),RAW_BUFFER_LENGTH);
        EXPECT_EQ(m_configreader->GetDecodedBufferLength(),DECODED_BUFFER_LENGTH);
        
        vector<int32> ref_threadnumber={3,3,6};
        EXPECT_EQ(m_configreader->GetDecodingThreadNumbers(),ref_threadnumber);
        EXPECT_EQ(m_configreader->GetCriticalShutterTime(),2.1);
        EXPECT_EQ(m_configreader->GetSlaveModule(),false);
    }

    TEST_F(ConfigReaderTest,TestLoadLocalConfigChangeMode)
    {
        m_configreader->LoadLocalConfig(false,"");
        m_configreader->LoadLocalConfig(true,"TwentyFourBit");
        
        vector<vector<string>> maclist,iplist;
        vector<uint16> portlist;
        m_configreader->GetUDPConfig(maclist,iplist,portlist);

        vector<vector<string>> ref_maclist
            = { { "90:B1:1C:3A:2C:D7", "00:0c:f1:f4:b0:4d" },
                { "B8:CA:3A:EE:6B:A7", "00:0c:f1:f4:b0:4d" } };

        vector<vector<string>> ref_iplist
            = { { "196.254.1.4", "196.254.1.41" },
                { "196.254.3.7", "196.254.3.41" } };

        vector<uint16> ref_portlist = { 4320, 4321, 4422 };
        
        EXPECT_EQ(maclist,ref_maclist);
        EXPECT_EQ(iplist,ref_iplist);
        EXPECT_EQ(portlist,ref_portlist);
        
        string tcpip;
        int16 tcpport;
        m_configreader->GetTCPConfig(tcpip,tcpport);

        string ref_tcpip = string("169.254.1.1");
        int16 ref_tcpport = 4321;
        EXPECT_EQ(ref_tcpip,tcpip);
        EXPECT_EQ(ref_tcpport,tcpport);
        
        EXPECT_EQ(m_configreader->GetMultilink(),true);

        EXPECT_EQ(m_configreader->GetBurstMode(),true);

        vector<int16> current_chips;
        vector<stMedipixChipData> chip_data;
        m_configreader->GetChipConfig(current_chips,chip_data);
        EXPECT_EQ(m_configreader->GetOperationMode(),"TwentyFourBit");
        EXPECT_EQ(m_configreader->GetModuleName(),"FullRXModule7");

        stDetCfgData config_data = m_configreader->GetDetConfigData();
        int32 ref_x = 1556;
        int32 ref_y = 516;
        szt ref_size = ref_x*ref_y;
        
        EXPECT_EQ((m_configreader->GetPixelMask()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetIndexFile()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetNominatorFile()).size(),ref_size);

        vector<float> ref_position(3,-1.0);
        EXPECT_EQ(m_configreader->GetPosition(),ref_position);

        int32 x,y;
        m_configreader->GetDistoredImageSize(x,y);
        EXPECT_EQ(x,ref_x);
        EXPECT_EQ(y,ref_y);
        
        EXPECT_EQ(m_configreader->GetRawBufferLength(),RAW_BUFFER_LENGTH);
        EXPECT_EQ(m_configreader->GetDecodedBufferLength(),DECODED_BUFFER_LENGTH);
        
        vector<int32> ref_threadnumber={3,3,6};
        EXPECT_EQ(m_configreader->GetDecodingThreadNumbers(),ref_threadnumber);
        EXPECT_EQ(m_configreader->GetCriticalShutterTime(),2.1);
        EXPECT_EQ(m_configreader->GetSlaveModule(),false);
    }

    TEST_F(ConfigReaderTest,TestLoadLocalConfigNonExistMode)
    {
        m_configreader->LoadLocalConfig(false,"");
        m_configreader->LoadLocalConfig(true,"NoneExistMode");
        
        vector<vector<string>> maclist,iplist;
        vector<uint16> portlist;
        m_configreader->GetUDPConfig(maclist,iplist,portlist);

        vector<vector<string>> ref_maclist
            = { { "90:B1:1C:3A:2C:D7", "00:0c:f1:f4:b0:4d" },
                { "B8:CA:3A:EE:6B:A7", "00:0c:f1:f4:b0:4d" } };

        vector<vector<string>> ref_iplist
            = { { "196.254.1.4", "196.254.1.41" },
                { "196.254.3.7", "196.254.3.41" } };

        vector<uint16> ref_portlist = { 4320, 4321, 4422 };
        
        EXPECT_EQ(maclist,ref_maclist);
        EXPECT_EQ(iplist,ref_iplist);
        EXPECT_EQ(portlist,ref_portlist);
        
        string tcpip;
        int16 tcpport;
        m_configreader->GetTCPConfig(tcpip,tcpport);

        string ref_tcpip = string("169.254.1.1");
        int16 ref_tcpport = 4321;
        EXPECT_EQ(ref_tcpip,tcpip);
        EXPECT_EQ(ref_tcpport,tcpport);
        
        EXPECT_EQ(m_configreader->GetMultilink(),true);

        EXPECT_EQ(m_configreader->GetBurstMode(),true);

        vector<int16> current_chips;
        vector<stMedipixChipData> chip_data;
        m_configreader->GetChipConfig(current_chips,chip_data);
        EXPECT_EQ(m_configreader->GetOperationMode(),"NoneExistMode");
        EXPECT_EQ(m_configreader->GetModuleName(),"FullRXModule7");

        stDetCfgData config_data = m_configreader->GetDetConfigData();
        int32 ref_x = 1556;
        int32 ref_y = 516;
        szt ref_size = ref_x*ref_y;
        
        EXPECT_EQ((m_configreader->GetPixelMask()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetIndexFile()).size(),ref_size);
        EXPECT_EQ((m_configreader->GetNominatorFile()).size(),ref_size);

        vector<float> ref_position(3,-1.0);
        EXPECT_EQ(m_configreader->GetPosition(),ref_position);

        int32 x,y;
        m_configreader->GetDistoredImageSize(x,y);
        EXPECT_EQ(x,ref_x);
        EXPECT_EQ(y,ref_y);
        
        EXPECT_EQ(m_configreader->GetRawBufferLength(),RAW_BUFFER_LENGTH);
        EXPECT_EQ(m_configreader->GetDecodedBufferLength(),DECODED_BUFFER_LENGTH);
        
        vector<int32> ref_threadnumber={3,3,6};
        EXPECT_EQ(m_configreader->GetDecodingThreadNumbers(),ref_threadnumber);
        EXPECT_EQ(m_configreader->GetCriticalShutterTime(),2.1);
        EXPECT_EQ(m_configreader->GetSlaveModule(),false);
    }
}


