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

#include <fsdetector/core/Globals.h>
#include "Version.h"

namespace DetLambdaNS
{
    using namespace FSDetCoreNS;

    ///lambda control ip address for TCP control socket
    const string TCP_CONTROL_IP_ADDRESS = "169.254.1.1";

    ///lambda udp sockets ip address
    const string UDP_CONTROL_IP_ADDRESS = "196.254.1.4";
    const string UDP_CONTROL_IP_ADDRESS_1 = "196.254.3.7";

    ///lambda control  port
    const short TCP_CONTROL_PORT = 4321;

    ///lambda udp socket port
    const short UDP_PORT = 4321;
    const short UDP_PORT_1 = 4422;

    ///threads number
    const int THREAD_NUMBER = 20;
    const bool MULTI_LINK = false;

    ///image buffer length(unit:image numbers)
    const int RAW_BUFFER_LENGTH = 100100;
    const int DECODED_BUFFER_LENGTH = 200;

    ///configuration files
    const string SYSTEM_CONFIG_FILE = "SystemConfig.txt";
    const string DETECTOR_CONFIG_FILE = "LambdaConfig.txt";

    ///command length
    const int COMMAND_LENGTH = 16;

    const int DAC_LENGTH = 36;

    const int OMR_LENGTH = 16;

    ///chip related parameter
    const int STANDARD_CHIP_NUMBERS = 12;

    const int BLOCK_SIZE_IN_BYTES = 32;

    const int CHIP_SIZE = 256;

    const int PIXELS_IN_CHIP = CHIP_SIZE*CHIP_SIZE;

    ///each pixel has 12bit so the bytes size is PIXELS_IN_CHIP*12/8
    const int BYTES_IN_CHIP = PIXELS_IN_CHIP*3/2;

    const int IMAGE_LENGTH_12_CHIP_IN_BYTE = 1180032;
    const int IMAGE_LENGTH_6_CHIP_IN_BYTE = 590016;

    const int PACKET_NUMBERS_12_CHIP = 133;
    const int PACKET_NUMBERS_6_CHIP = 67;

    const int CHIP_HEADER_SIZE = 32; //bytes

    const int DEFAULT_COMPRESSION_CHUNK = 131072;

    enum Enum_readout_mode
    {
        OPERATION_MODE_12,                    /**< enum value 0 */
        OPERATION_MODE_2x12,
        OPERATION_MODE_24,
        OPERATION_MODE_UNKNOWN
    };


    ///////////////////////////////////////////////////////////////////////////
    ///Medipix chip data
    ///////////////////////////////////////////////////////////////////////////
    /**
     * @brief Medipix chip data
     */
    struct stMedipixChipData
    {
        short shPreamp;
        short shIkrum;
        short shShaper;
        short shDisc;

        short shDiscLS;
        short shDACDiscL;
        short shDACDiscH;
        short shDelay;

        short shTPBufferIn;
        short shTPBufferOut;
        short shRPZ;
        short shGND;

        short shTPREF;
        short shFBK;
        short shCAS;
        short shTPREFA;

        short shTPREFB;

        string strSenseDAC;
        string strExtDAC;

        vector<unsigned char> vStrConfig0;
        vector<unsigned char> vStrConfig1;
        vector<unsigned char> vStrDAC;
        vector<unsigned char> vStrOMR;
        vector<short> vThreshold;

        vector<short> vTestBit;
        vector<short> vMaskBit;
        vector<short> vConfigTHA; //size is PIXELS_IN_CHIP
        vector<short> vConfigTHB; //Ssize is PIXELS_IN_CHIP

        vector<short> vLatestChipImage;
        /// used to convert keV threshold values given to the chip to THL settings
        vector<float> vKeVToThrSlope;
        /// used to convert keV settings to thresholds
        /// this is the base line threshold setting corresponding to 0 keV
        vector<float> vThrBaseline;

    stMedipixChipData()
    :shPreamp(100),
            shIkrum(20),
            shShaper(150),
            shDisc(150),
            shDiscLS(200),
            shDACDiscL(0),
            shDACDiscH(0),
            shDelay(128),
            shTPBufferIn(50),
            shTPBufferOut(128),
            shRPZ(255),
            shGND(100),
            shTPREF(0),
            shFBK(100),
            shCAS(140),
            shTPREFA(511),
            shTPREFB(255),
            strSenseDAC("none"),
            strExtDAC("none")
            {
                int nPhysicalCounterSize = 12;
                int nThresholds = 8;
                vStrConfig0.resize(PIXELS_IN_CHIP*nPhysicalCounterSize/8,0x00);
                vStrConfig1.resize(PIXELS_IN_CHIP*nPhysicalCounterSize/8,0x00);
                vStrDAC.resize(DAC_LENGTH,0x00);
                vStrOMR.resize(OMR_LENGTH,0x00);
                vThreshold.resize(nThresholds,500);
                vTestBit.resize(PIXELS_IN_CHIP,0);
                vMaskBit.resize(PIXELS_IN_CHIP,0);
                vConfigTHA.resize(PIXELS_IN_CHIP,0);
                vConfigTHB.resize(PIXELS_IN_CHIP,0);
                vLatestChipImage.resize(PIXELS_IN_CHIP);
                vKeVToThrSlope.resize(nThresholds,0);
                vThrBaseline.resize(nThresholds,10.);
            }
    };

    /**
     * @brief detector configuration
     */
    struct stDetCfgData
    {
        vector<int> vThresholdScan;
        int nCounterMode;

        short shCounterBitDepth;
        short shTriggerMode;
        short shRowBlockSel;
        short shGainMode;

        bool bCRW;
        bool bPolarity;
        bool bEnableTP;
        bool bEqualise;

        bool bDiscSPMCSM;
        bool bOMRHeader;
        bool bColorMode;
        bool bChargeSum;

        short shDataOutLines;

    stDetCfgData()
    :nCounterMode(0),
            shCounterBitDepth(12),
            shTriggerMode(0),
            shRowBlockSel(0),
            shGainMode(0),
            bCRW(false),
            bPolarity(true),
            bEnableTP(false),
            bEqualise(false),
            bDiscSPMCSM(false),
            bColorMode(false),
            bChargeSum(false),
            shDataOutLines(0)
            {
                vThresholdScan.resize(1,0);
            }
    };

    ///////////////////////////////////////////////////////////////////////////
    ///end of Medipix chip data
    ///////////////////////////////////////////////////////////////////////////
}
