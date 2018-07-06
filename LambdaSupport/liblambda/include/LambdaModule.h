/*
 * (c) Copyright 2014-2017 DESY, Yuelong Yu <yuelong.yu@desy.de>
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

#pragma once

#include "LambdaGlobals.h"
#include "LambdaConfigReader.h"
#include <fsdetector/core/NetworkInterface.h>
#include <fsdetector/core/NetworkImplementation.h>
#include <fsdetector/core/Utils.h>

namespace DetLambdaNS
{
    using namespace FSDetCoreNS;
    
    class LambdaModule
    {
      public:
        /**
         * @brief constructor
         */
        LambdaModule();

        /**
         * @brief init all module parameters
         */

        bool InitModule(NetworkInterface* objTCP,LambdaConfigReader& objConfig);

        /**
         * @brief config data link
         */
        void ConfigDataLink();
        
        /**
         * @brief get firmware version
         * @return firmware version. Default: unknown
         */
        string GetFirmwareVersion();

        /**
         * @brief set shutter time
         * @param dTime shutter time
         */
        void WriteShutterTime(double dTime);

        /**
         * @brief set delay time
         * @param dTime delay time
         */
        void WriteDelayTime(double dTime);

        /**
         * @brief set trigger mode
         * @param shTriggerMode trigger mode
         */
        void WriteTriggerMode(int16 shTriggerMode);
            
        /**
         * @brief set image numbers which need to be acquired
         */
        void WriteImageNumbers(int32 lImgNo);

        /**
         * @brief set engergy threshold
         * @param nThresholdNo from 0-7
         * @param fEnergy energy val
         */
        void WriteEnergyThreshold(int32 nThresholdNo, float fEnergy);
            
        /**
         * @brief set network mode
         * @param nMode 10GE mode, 0:1GE mode
         */
        void WriteNetworkMode(int32 nMode);

        /**
         * @brief send MAC address  to detector
         * @param nCH 10GE link selector, either 0 or 1
         * @param vStrMAC MAC address list. The order is:\n
         *        0:destination MAC address
         *        1:source MAC address
         */
        void WriteUDPMACAddress(int32 nCH,vector<string> vStrMAC);

        /**
         * @brief send UP address configuration to detector
         * @param nCH 10GE link selector, either 0 or 1
         * @param vStrIP IP address list. The order is:\n
         *        0:destination IP address
         *        1:source IP address
         */
        void WriteUDPIP(int32 nCH,vector<string> vStrIP);
        
        /**
         * @brief send udp ports to detector
         * @param nCH 10GE link selector ,either 0 or 1
         * @param vUShPort port number list. The order is:\n
         *        0:source port;\n
         *        1:destination port 0;\n
         *        2:destination port 1;
         */
        void WriteUDPPorts(int32 nCH,vector<uint16> vUShPort);
        
        /**
         * @brief set read out mode
         * @param nMode mode
         */
        void WriteReadOutMode(int32 nMode);

        /**
         * @brief setup detector for fast image
         */
        void SetupFastImaging();


        /**
         * @brief Prepare detector for next image series -
         * should be done (A) after changing threshold / matrix config,
         * and (B) after finishing an image series. 
         */
        void PrepNextImaging();
	
        /**
         * @brief  This will automatically start taking a series of images.
         * In the current version, this software takes control.
         * In the future, this should be changed
         * so that the image taking is controlled by the Lambda module,
         * and this function simply sends a command to start the process.
         */

        void StartFastImaging();
        void StopFastImaging();

        /**
         * @brief reset detector configurations 
         */
        void Reset();
            
        /**
         * @brief set DAC
         * @param nChipNo chip No (from 1-12)
         */
        void SetDAC(int32 nChipNo);

        void SetAllDACs();
       
        /**
         * @brief update DAC string
         * @param nChipNo chip No
         */
        void UpdateDACString(int32 nChipNo);
     
        /**
         * @brief update config string
         * @param nChipNo chip No
         */
        void UpdateConfigStrings(int32 nChipNo);

        /**
         * @brief config all matrices
         */
        void ConfigAllMatrices();
        
        bool LoadMatrixConfigRXHack(int32 nChipNo,
                                    vector<uchar> vStrConfigData,
                                    int32 nCounterNo);
                      
        void CreateMatrixConfig1(int32 nChipNo);

        /**
         * @brief  To tell a chip to monitor or override a certain DAC,
         * we need to insert a corresponding 5-bit code into the OMR string.
         * This converts the DAC name as a string to the code value.
         */
        int16 FindDACCode(string strDACname);

        /**
         * @brief get one specific bit from data
         * @param shData source data
         * @param nBit position
         * @return the value of the bit
         */
        uchar Grabbit(int16 shData,int32 nBit);
            
        /**
         * @brief update OMR string
         * @param nChipNo chip No
         */
        void UpdateOMRString(int32 nChipNo);

        /**
         * @brief load and check matrix config
         */
        void LoadAndCheckMatrixConfig(int32 nChipNo);

        /**
         * @brief matrix fast clear
         */
        void MatrixFastClear();

        /**
         * @brief Read back OMR string from detector;
         * this makes it possible to extract chip ID
         */
        vector<char> ReadOMR(int32 nChipNo);

        /**
         * @brief get chip id
         * @param chip_no chip No.
         * @return chip id
         */

        string GetChipID(int32 chip_no);

         /*            
         * @brief Wait for response from detector after command
         * This should help timing issues and check if commands executed OK
         * @return Response -1 for timeout, 0 for failure, 1 for success
         */

        int CheckResponse();
        
        /**
         * @brief destructor
         */
         ~LambdaModule();

      private:
            
        /**
         * @brief enable chip
         * @param chip no
         */
        void EnableChip(int32 nChipNo);

      private:
        string m_strModuleID;
        string m_strSystemType;

        NetworkInterface* m_objNetTCPInterface;

        // used chips in current operation mode
        // e.g. 6 chips are used in some operation modes
        // these values can be read from local config file
        vector<int16> m_vCurrentUsedChips;
        stDetCfgData m_stDetCfg;
        vector<stMedipixChipData> m_vStChips;
        bool m_bSlaveModule;
        
        double m_dShutterTime;
        int16 m_shTriggerMode;
        int32 m_lImageNo;
        int32 m_nThresholdToScan;
        char* m_ptrchExeString;
        vector<uchar> m_vExeCmd;
        int32 m_nChips;
        StringUtils* m_objStrUtil;
        string m_strTCPIPAddress;
        int16 m_shTCPPortNo;
        
        // data receiver IPs and ports
        vector< vector<string> > m_vStrMACs;
        vector<vector<string>> m_vStrIPs;
        vector<uint16> m_usPorts;
    };///end of class LambdaModule
}
