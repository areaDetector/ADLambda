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

#ifndef __LAMBDA_MODULE_H__
#define __LAMBDA_MODULE_H__

#include "LambdaGlobals.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    class NetworkInterface;
    class StringUtils;
        
    class LambdaModule
    {
      public:
        /**
         * @brief constructor
         */
        LambdaModule();

        /**
         * @brief contructor
         * @param _strModuleID module id
         * @param _objNetIn TCP network interface for controlling command
         * @param _bMultilink multlink or not
         * @param _vCurrentChips current used chips
         * @param _stDetCfg detector config data
         * @param _vStChipData for all current used chip data
         */
        LambdaModule(string _strModuleID, NetworkInterface* _objNetIn,bool _bMultilink, vector<short> _vCurrentChips, stDetCfgData _stDetCfg, vector<stMedipixChipData> _vStChipData, bool _bSlaveModule);

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
        void WriteTriggerMode(short shTriggerMode);
            
        /**
         * @brief set image numbers which need to be acquired
         */
        void WriteImageNumbers(long lImgNo);

        /**
         * @brief set engergy threshold
         * @param nThresholdNo from 0-7
         * @param fEnergy energy val
         */
        void WriteEnergyThreshold(int nThresholdNo, float fEnergy);
            
        /**
         * @brief set network mode
         * @param nMode 10GE mode, 0:1GE mode
         */
        void WriteNetworkMode(int nMode);

        /**
         * @brief send MAC address  to detector
         * @param nCH 10GE link selector, either 0 or 1
         * @param vStrMAC MAC address list. The order is:\n
         *        0:destination MAC address
         *        1:source MAC address
         */
        void WriteUDPMACAddress(int nCH,vector<string> vStrMAC);

        /**
         * @brief send UP address configuration to detector
         * @param nCH 10GE link selector, either 0 or 1
         * @param vStrIP IP address list. The order is:\n
         *        0:destination IP address
         *        1:source IP address
         */
        void WriteUDPIP(int nCH,vector<string> vStrIP);
        
        /**
         * @brief send udp ports to detector
         * @param nCH 10GE link selector ,either 0 or 1
         * @param vUShPort port number list. The order is:\n
         *        0:source port;\n
         *        1:destination port 0;\n
         *        2:destination port 1;
         */
        void WriteUDPPorts(int nCH,vector<unsigned short> vUShPort);
        
       
        /**
         * @brief set read out mode
         * @param nMode mode
         */
        void WriteReadOutMode(int nMode);

        /**
         * @brief setup detector for fast image
         */
        void SetupFastImaging();


        /**
         * @brief Prepare detector for next image series - should be done (A) after changing threshold / matrix config, and (B) after finishing an image series. 
         */
        void PrepNextImaging();
	
        /**
         * @brief  This will automatically start taking a series of images. In the current version, this software takes control. In the future, this should be changed so that the image taking is controlled by the Lambda module, and this function simply sends a command to start the process.
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
        void SetDAC(int nChipNo);

        void SetAllDACs();
       
        /**
         * @brief update DAC string
         * @param nChipNo chip No
         */
        void UpdateDACString(int nChipNo);
     
        /**
         * @brief update config string
         * @param nChipNo chip No
         */
        void UpdateConfigStrings(int nChipNo);

        /**
         * @brief config all matrices
         */
        void ConfigAllMatrices();
        
        bool LoadMatrixConfigRXHack(int nChipNo,vector<unsigned char> vStrConfigData,int nCounterNo);
                      
        void CreateMatrixConfig1(int nChipNo);

        /**
         * @brief  To tell a chip to monitor or override a certain DAC, we need to insert a corresponding 5-bit code into the OMR string. This converts the DAC name as a string to the code value.
         */
        short FindDACCode(string strDACname);

        /**
         * @brief get one specific bit from data
         * @param shData source data
         * @param nBit position
         * @return the value of the bit
         */
        unsigned char Grabbit(short shData,int nBit);

        /**
         * @brief get module id
         * @return module name
         */
        string GetModuleID() const;

        /**
         * @brief set module id
         * @param strModuleID module ID
         */
        void SetModuleID(const string strModuleID);
            
        /**
         * @brief update OMR string
         * @param nChipNo chip No
         */
        void UpdateOMRString(int nChipNo);

        /**
         * @brief load and check matrix config
         */
        void LoadAndCheckMatrixConfig(int nChipNo);

        /**
         * @brief matrix fast clear
         */
        void MatrixFastClear();

       /**
         * @brief Read back OMR string from detector; this makes it possible to extract chip ID
         */
        vector<char> ReadOMR(int nChipNo);
        
        /**
         * @brief destructor
         */
        virtual ~LambdaModule();

      private:
        /**
         * @brief init all module parameters
         */
        void InitModule();
            
        /**
         * @brief enable chip
         * @param chip no
         */
        void EnableChip(int nChipNo);

      private:
        vector<stMedipixChipData> m_vStChips;

        // used chips in current operation mode
        // e.g. 6 chips are used in some operation modes
        // these values can be read from local config file
        vector<short> m_vCurrentUsedChips;
        stDetCfgData m_stDetCfg;
        string m_strModuleID;
        NetworkInterface* m_objNetTCPInterface;
        double m_dShutterTime;
        short m_shTriggerMode;
        long m_lImageNo;
        int m_nThresholdToScan;
        char* m_ptrchExeString;
        vector<unsigned char> m_vExeCmd;
        int m_nChips;
        StringUtils* m_objStrUtil;
        bool m_bMultilink;
	bool m_bSlaveModule;
        
    };///end of class LambdaModule
}///end of namespace DetCommonNS
#endif
