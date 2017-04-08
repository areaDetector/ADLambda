/*
 * (c) Copyright 2014-2015 DESY, Yuelong Yu <yuelong.yu@desy.de>
 *
 * This file is part of FS-DS detector library.
 *
 * This software is free: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************
 *     Author: Yuelong Yu <yuelong.yu@desy.de>
 */

#ifndef __MODULE_TEST_H__
#define __MODULE_TEST_H__

#include <iostream>
#include <memory>
#include <cppunit/extensions/HelperMacros.h>

#include <Globals.h>
#include <LambdaGlobals.h>

namespace DetUnitTestNS
{

    using namespace DetCommonNS;

    class CheckEquality;
    
    class ModuleTest : public CppUnit::TestFixture
    {
        //add to test suite
        CPPUNIT_TEST_SUITE(ModuleTest);
        CPPUNIT_TEST(TestInit);
        CPPUNIT_TEST(TestShutterTime);
        //CPPUNIT_TEST(TestDelayTime);
        CPPUNIT_TEST(TestTriggerMode);
        CPPUNIT_TEST(TestImageNumbers);
        CPPUNIT_TEST(TestEnergyThreshold);
        //CPPUNIT_TEST(TestReadOutMode);
        //CPPUNIT_TEST(TestNetworkMode);
        CPPUNIT_TEST(TestUDPMACADDress);
        CPPUNIT_TEST(TestUDPIP);
        CPPUNIT_TEST(TestUDPPorts);
        // CPPUNIT_TEST(TestReset);
        CPPUNIT_TEST(TestStartFastImaging);
        CPPUNIT_TEST(TestStopFastImaging);
        CPPUNIT_TEST_SUITE_END();
        
      public:
        void setUp();
        void tearDown();

        void TestInit();
        void TestShutterTime();
        void TestDelayTime();
        void TestTriggerMode();
        void TestImageNumbers();
        void TestEnergyThreshold();
        void TestReadOutMode();
        void TestNetworkMode();
        void TestUDPMACADDress();
        void TestUDPIP();
        void TestUDPPorts();
        void TestReset();
        void TestStartFastImaging();
        void TestStopFastImaging();
        
      private:
        bool ReadConfig(bool bSwitchMode,string strOpMode);

      private:
        bool m_bMultiLink;
        short m_shTCPPortNo;
        string m_strFilePath;
        string m_strModuleID;
        string m_strTCPIPAddress;
        string m_strOperationMode;
        
        vector<char> m_vchCmd;
        vector<short> m_vshCurrentChips;
        vector<unsigned short> m_vushPort;
        vector< vector<string> > m_vstrMAC;
        vector< vector<string> > m_vstrIP;
        
        vector<stMedipixChipData> m_vstChipData;
        stDetCfgData m_stDetData;
       
        std::unique_ptr<LambdaModule> m_upModule;
        std::unique_ptr<LambdaConfigReader> m_upConfig;
        std::shared_ptr<NetworkInterface> m_spNetInt;
        std::shared_ptr<NetworkImplementation> m_spNetImp;
        std::unique_ptr<FileOperation> m_upFileOp;
        std::unique_ptr<CheckEquality> m_upCheckEqual;
    };
}

#endif
