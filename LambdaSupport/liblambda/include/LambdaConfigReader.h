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
#include <fsdetector/core/FilesOperation.h>
#include <fsdetector/core/Utils.h>

namespace DetLambdaNS
{
    using namespace FSDetCoreNS;

    class LambdaConfigReader
    {
      public:
        /**
         * @brief constructor
         */
        LambdaConfigReader();

        /**
         * @brief constructor
         * @param _strConfigFile config file path
         */
        LambdaConfigReader(string _strConfigFilePath);

        /**
         * @brief destructor
         */
        ~LambdaConfigReader();

        /**
         * @brief get udp configuration for 2x10GE links(CH0,CH1)
         * @return vStrMAC mac address\n
         *         vector index structure:\n
         *         0,0 : CH0 destination MAC address\n
         *         0,1 : CH0 src MAC address\n
         *         1,0 : CH1 desitnation MAC address\n
         *         1,1 : CH1 src MAC address
         *
         * @return vStrIP IP address\n
         *         vector index structure:\n
         *         0,0 : CH0 destination IP address\n
         *         0,1 : CH0 src IP address\n
         *         1,0 : CH1 desitnation IP address\n
         *         1,1 : CH1 src IP addres\n
         *
         * @return vUShPort port No\n
         *         vector index structure:\n
         *         0 : source port for both CH0, CH1
         *         1 : destination port No1 for both CH0,CH1
         *         2 : destination port No2 for both CH0,CH1
         *
         */
        void GetUDPConfig(vector<vector<string>>& vStrMAC,
                          vector<vector<string>>& vStrIP,
                          vector<uint16>& vUShPort);

        /**
         * @brief get tcp configuration
         * @return strTCPIPAddr TCP link IP address
         * @return shTCPPort TCP link port No
         */
        void GetTCPConfig(string& strTCPIPAddr,int16& shTCPPort);

        /**
         * @brief get multilink type
         * @return true multilink;false single link
         */
        bool GetMultilink() const;

        /**
         * @brief get "burst mode" (i.e. 1G TCP vs 10G UDP) type
         * @return true 10G UDP; false 1G TCP
         */
        bool GetBurstMode() const;

        /**
         * @brief get chip config
         * @return vCurrentUsedChips used chips in current detector module
         * @return vStCurrentChipData the chip config data of current used chips\n
         * @see DetCommonNS::stMedipixChipData in LambdaGlobals.h structure of stMedipixChipData
         */
        void GetChipConfig(vector<int16>& vCurrentUsedChips,
                           vector<stMedipixChipData>& vStCurrentChipData);

        /**
         * @brief get operation mode of detector
         * @return operation mode
         */
        string GetOperationMode();

        /**
         * @brief get module name
         * @return module name
         */

        string GetModuleName();
        /**
         * @brief get type of system (e.g. "small" for our new single chip)
         * @return system type. Note that this is aimed at allowing workarounds for firmware
         */
        string GetSystemType();

        /**
         * @brief get detector config data
         * @return detector config data
         */
        stDetCfgData GetDetConfigData();

        /**
         * @brief load all config files,include system config and chip config
         * @param bOpModeRunningSwitch switch mode when detector is running.\n
         *        True: detector is already running.false: detector first time initialized
         * @param strOpMode load specific operation mode configuration file
         */
        void LoadLocalConfig(bool bOpModeRunningSwitch,string strOpMode);

        /**
         * @brief get pixel mask
         * @return pixel mask
         */
        vector<uint32> GetPixelMask();

        /**
         * @brief get index file
         * @return index file
         */
        vector<int32> GetIndexFile();

        /**
         * @brief get nominator file
         * @return nominator file info
         */
        vector<int32> GetNominatorFile();

        /**
         * @brief translation information for multi module system
         * @return translation information vector index: 0:x;1:y;2:z
         */
        vector<float> GetPosition();

        /**
         * @brief get distorted image size
         * @return nX
         * @return nY
         */
        void GetDistoredImageSize(int32& nX,int32& nY);

        /**
         * @brief get maximum raw buffer length
         * @return raw buffer length
         */
        int32 GetRawBufferLength();


        /**
         * @brief get maximum decoded buffer length
         * @return decoded buffer length
         */
        int32 GetDecodedBufferLength();

        /**
         * @brief get number of decoding threads needed of each type
         * @return Vector lists no of HIGH, NORMAL and LOW priority decoding threads
         */
        vector<int32> GetDecodingThreadNumbers();

        /**
         * @brief get shutter time above which the NORMAL decoding threads
         * will run during image reception
         * @return critical shutter time
         */
        double GetCriticalShutterTime();

        /**
         * @brief get whether module is a slave module in a multimodule system
         * @return true if slave modules, false if not
         */
        bool GetSlaveModule();

        /**
        * @brief get chunk config of AHA compression card
        * @param nChunkIn return value of chunk in
        * @param nChunkOut return value of chunk output
        * @param nPostCode post code of aha card
        */
        void GetCompressionChunkConfig(int& nChunkIn,int& nChunkOut,int& nPostCode);

      private:
        ///private methods
        /**
         * @brief read lookup tables(pixel masks, lookups for distortion correction)
         */
        void GetDataFromLookupFolders();

        /**
         * @brief get system config content
         */
        void GetDataFromSystemConfig();

        /**
         * @brief get detector config
         */
        void GetDataFromDetConfig();

        /**
         * @brief get detector config
         */
        void GetDataFromChipConfig();

        /**
         * @brief extract information from original content from file
         * @param vStrSysCfg original content of file
         */
        void ExtractDataFromSysConfig(vector<string> vStrSysCfg);

        /**
         * @brief extract information from original content from file
         * @param vStrDetCfg original content of file
         */
        void ExtractDataFromDetConfig(vector<string> vStrDetCfg);

        /**
         * @brief extract information from original content from file
         * @param vStrChipCfg original content of file
         * @param nIdx chip index from 0-11
         */
        void ExtractDataFromChipConfig(vector<string> vStrChipCfg,int32 nIdx);

        ///private member variables

        //configuration file related
        string m_strConfigFilePath;
        string m_strSystemConfigFile;
        string m_strDetConfigFile;

        ///network related configuration
        vector< vector<string> > m_vStrMAC;
        vector< vector<string> > m_vStrIP;
        vector<uint16> m_vUShPort;;

        vector<float> m_vfTranslation;

        //final image size
        int32 m_nX;
        int32 m_nY;

        //maximum raw image numbers
        int32 m_nMaxRawImageNumbers;

        //maximum decoded image numbers
        int32 m_nMaxDecodedImageNumbers;

        // Slave module check (for multimodule systems
        bool m_bSlaveModule;

        bool m_bMultilink;
        bool m_bBurstMode;


        string m_strTCPIPAddress;
        int16 m_shTCPPortNo;

        //No of decoding threads of different priorities (HIGH, NORMAL, LOW)
        vector<int32> m_vNDecodingThreads;

        string m_strOperationMode;
        string m_strCurrentModuleName;
        string m_strSystemType;
        ///chip related configuration
        stDetCfgData m_stDetCfgData;
        vector<int16> m_vCurrentChip;
        vector<stMedipixChipData> m_vStCurrentChipData;

        vector<uint32> m_vUnPixelMask;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;

        // Shutter time where we switch decoding thread strategy
        double m_dCriticalShutterTime;

        FileOperation* m_objFileOp;

        ///helper class object
        StringUtils* m_objStrUtil;

        int m_nChunkIn,m_nChunkOut,m_nPostCode;
    };///end of class LambdaConfigReader
}
