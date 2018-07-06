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

#include "LambdaModule.h"

namespace DetLambdaNS
{
    LambdaModule::LambdaModule()
        :m_dShutterTime(2000),
         m_shTriggerMode(0),
         m_lImageNo(1)
    {
        LOG_TRACE(__FUNCTION__); 

        //init module
        //InitModule();      
    }
        
    LambdaModule::~LambdaModule()
    {
        LOG_TRACE(__FUNCTION__);
        
        m_vCurrentUsedChips.clear();
        m_vStChips.clear();
    }

    bool LambdaModule::InitModule(NetworkInterface* objTCP,LambdaConfigReader& objConfig)
    {
        LOG_TRACE(__FUNCTION__);

        // read configuration
        objConfig.GetTCPConfig(m_strTCPIPAddress,m_shTCPPortNo);
        objConfig.GetChipConfig(m_vCurrentUsedChips,m_vStChips);
        m_stDetCfg = objConfig.GetDetConfigData();
        m_bSlaveModule = objConfig.GetSlaveModule();
        objConfig.GetUDPConfig(m_vStrMACs,m_vStrIPs,m_usPorts);
        m_strSystemType = objConfig.GetSystemType();
        
        m_objNetTCPInterface = objTCP;
        
        m_vExeCmd.resize(COMMAND_LENGTH,0x00);
        m_vExeCmd[1] = 0xa0;


        // Workaround for 6 chip to avoid unwanted load commands
        // Currently commented out to test new 12-chip handshake
        //if(m_strSystemType == "handshake")
        //    m_vCurrentUsedChips = {1, 2, 3, 10, 11, 12};

        // initialize all the parameters
        for(szt i=0;i<m_vCurrentUsedChips.size();i++)
        {
            int32 nChipNo = m_vCurrentUsedChips[i];
                
            UpdateOMRString(nChipNo);
            UpdateDACString(nChipNo);
            UpdateConfigStrings(nChipNo);
        }
        SetupFastImaging();

       

        return true;
    }

    void LambdaModule::ConfigDataLink()
    {
        //send udp data via TCP
       WriteUDPMACAddress(0,m_vStrMACs[0]);
       WriteUDPMACAddress(1,m_vStrMACs[1]);

       WriteUDPIP(0,m_vStrIPs[0]);
       WriteUDPIP(1,m_vStrIPs[1]);
            
       WriteUDPPorts(0,m_usPorts);
       WriteUDPPorts(1,m_usPorts);
    }

    string LambdaModule::GetFirmwareVersion()
    {
        return string("unknown");
    }
        
    void LambdaModule::WriteShutterTime(double dTime)
    {
        LOG_TRACE(__FUNCTION__);

        m_dShutterTime = dTime;
            
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        unsigned long lSelector = 255;
        long lClockticks = (long)(dTime*100000);//find corresponding 100MHz clock ticks for time in ms
            
        vCmd[1] = 0xf0;
        vCmd[2] = 0x46;

        vCmd[3] = (lClockticks>>56 & lSelector);
        vCmd[4] = (lClockticks>>48 & lSelector);
        vCmd[5] = (lClockticks>>40 & lSelector);
        vCmd[6] = (lClockticks>>32 & lSelector);
        vCmd[7] = (lClockticks>>24 & lSelector);
        vCmd[8] = (lClockticks>>16 & lSelector);
        vCmd[9] = (lClockticks>>8 & lSelector);
        vCmd[10] = (lClockticks & lSelector);        
        
        m_objNetTCPInterface->SendData(vCmd);
    }

    void LambdaModule::WriteDelayTime(double dTime)
    {
        LOG_TRACE(__FUNCTION__);
        ///TODO
        ///Currently not used
        ///implement later
    }

    void LambdaModule::WriteTriggerMode(int16 shTriggerMode)
    {
        LOG_TRACE(__FUNCTION__);

        m_shTriggerMode = shTriggerMode;
            
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x48;
        vCmd[3] = shTriggerMode;
        m_objNetTCPInterface->SendData(vCmd);
    }
    
    void LambdaModule::WriteImageNumbers(int32 lImgNo)
    {
        LOG_TRACE(__FUNCTION__);

        m_lImageNo = lImgNo;
            
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        uint32 nSelector = 255;
            
        vCmd[1] = 0xf0;
        vCmd[2] = 0x47;
        vCmd[3] = (lImgNo>>16 & nSelector);
        vCmd[4] = (lImgNo>>8 & nSelector);
        vCmd[5] = (lImgNo & nSelector);
            
        m_objNetTCPInterface->SendData(vCmd);
    }

    void LambdaModule::WriteEnergyThreshold(int32 nThresholdNo, float fEnergy)
    {
        LOG_TRACE(__FUNCTION__);
        float fEng;
        int16 maxVal = 511; // Maximum value of DAC
        int16 sEng;
            
        for(szt i=0;i<m_vCurrentUsedChips.size();i++)
        {
            fEng = m_vStChips[m_vCurrentUsedChips[i]-1].vKeVToThrSlope[nThresholdNo]*fEnergy
                + m_vStChips[m_vCurrentUsedChips[i]-1].vThrBaseline[nThresholdNo];
            sEng = static_cast<int16>(fEng);
            if(sEng > maxVal) sEng = maxVal;
                
            m_vStChips[m_vCurrentUsedChips[i]-1].vThreshold[nThresholdNo] = sEng;
                
        }
        //send new data to detector
        SetAllDACs();
        PrepNextImaging();
    }

    void LambdaModule::WriteReadOutMode(int32 nMode)
    {
        LOG_TRACE(__FUNCTION__);
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x49;
        vCmd[3] = nMode;
        m_objNetTCPInterface->SendData(vCmd);
    }
        
    void LambdaModule::WriteNetworkMode(int32 nMode)
    {
        LOG_TRACE(__FUNCTION__);
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x50;
        vCmd[3] = nMode;
        m_objNetTCPInterface->SendData(vCmd);
        if(nMode==0)
            std::cout << "Setting network mode to 0" << "\n";
    }
    
    void LambdaModule::WriteUDPMACAddress(int32 nCH,vector<string> vStrMAC)
    {
        LOG_TRACE(__FUNCTION__);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vector<uint32> vShMAC(6,0);
        vCmd[1] = 0xf0;
        
        //size check
        if(vStrMAC.size()!=2)
            LOG_STREAM(__FUNCTION__,ERROR,"MAC address length error!!!");
        
        //decide which 10GE link
        if(nCH == 0)
        {
            //destination MAC CH0
            vCmd[2] = 0x54;
            sscanf(vStrMAC[0].c_str(),
                   "%x:%x:%x:%x:%x:%x",
                   &vShMAC[0],&vShMAC[1],&vShMAC[2],&vShMAC[3],&vShMAC[4],&vShMAC[5]);
            
            for(szt i=0;i<vShMAC.size();i++)
                vCmd[i+3] = (uchar)(vShMAC[i]);
            m_objNetTCPInterface->SendData(vCmd);
             
            //src MAC CH0
            vCmd[2] = 0x55;
            sscanf(vStrMAC[1].c_str(),
                   "%x:%x:%x:%x:%x:%x",
                   &vShMAC[0],&vShMAC[1],&vShMAC[2],&vShMAC[3],&vShMAC[4],&vShMAC[5]);
            
            for(szt i=0;i<vShMAC.size();i++)
                vCmd[i+3] = (uchar)(vShMAC[i]);
            m_objNetTCPInterface->SendData(vCmd);
        }
        else if(nCH == 1)
        {
            //destination MAC CH1
            vCmd[2] = 0x56;
            sscanf(vStrMAC[0].c_str(),
                   "%x:%x:%x:%x:%x:%x",
                   &vShMAC[0],&vShMAC[1],&vShMAC[2],&vShMAC[3],&vShMAC[4],&vShMAC[5]);
            
            for(szt i=0;i<vShMAC.size();i++)
                vCmd[i+3] = (uchar)(vShMAC[i]);
            m_objNetTCPInterface->SendData(vCmd);

            //src MAC CH1
            vCmd[2] = 0x57;
            sscanf(vStrMAC[1].c_str(),
                   "%x:%x:%x:%x:%x:%x",
                   &vShMAC[0],&vShMAC[1],&vShMAC[2],&vShMAC[3],&vShMAC[4],&vShMAC[5]);
            
            for(szt i=0;i<vShMAC.size();i++)
                vCmd[i+3] = (uchar)(vShMAC[i]);
            
            m_objNetTCPInterface->SendData(vCmd);
        }
        
    }
    
    void LambdaModule::WriteUDPIP(int32 nCH,vector<string> vStrIP)
    {
        LOG_TRACE(__FUNCTION__);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vector<uint32> vShIP(4,0);
        vCmd[1] = 0xf0;
        
        //size check
        if(vStrIP.size()!=2)
            LOG_STREAM(__FUNCTION__,ERROR,"IP address length error!!!");
        
        //decide which 10GE link
        if(nCH == 0)
        {
            //destination IP CH0
            vCmd[2] = 0x58;
            sscanf(vStrIP[0].c_str(),"%d.%d.%d.%d",&vShIP[0],&vShIP[1],&vShIP[2],&vShIP[3]);
            for(szt i=0;i<vShIP.size();i++)
                vCmd[i+3] = (uchar)(vShIP[i]);
            m_objNetTCPInterface->SendData(vCmd);
            
            //src IP CH0
            vCmd[2] = 0x59;
            sscanf(vStrIP[1].c_str(),"%d.%d.%d.%d",&vShIP[0],&vShIP[1],&vShIP[2],&vShIP[3]);
            for(szt i=0;i<vShIP.size();i++)
                vCmd[i+3] = (uchar)(vShIP[i]);
            m_objNetTCPInterface->SendData(vCmd);
        }
        else if(nCH == 1)
        {
            //destination IP CH1
            vCmd[2] = 0x5a;
            sscanf(vStrIP[0].c_str(),"%d.%d.%d.%d",&vShIP[0],&vShIP[1],&vShIP[2],&vShIP[3]);
            for(szt i=0;i<vShIP.size();i++)
                vCmd[i+3] = (uchar)(vShIP[i]);
            m_objNetTCPInterface->SendData(vCmd);

            //src IP CH1
            vCmd[2] = 0x5b;
            sscanf(vStrIP[1].c_str(),"%d.%d.%d.%d",&vShIP[0],&vShIP[1],&vShIP[2],&vShIP[3]);
            for(szt i=0;i<vShIP.size();i++)
                vCmd[i+3] = (uchar)(vShIP[i]);
            m_objNetTCPInterface->SendData(vCmd);
        }
    }
    
    void LambdaModule::WriteUDPPorts(int32 nCH,vector<uint16> vUShPort)
    {
        LOG_TRACE(__FUNCTION__);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;

        //decide which 10GE link
        if(nCH == 0)
            vCmd[2] = 0x5c;
        else if(nCH == 1)
            vCmd[2] = 0x5d;

        // verify port number size
        if(vUShPort.size() != 3)
            LOG_STREAM(__FUNCTION__,ERROR,"port number size does not match");

        //source port
        vCmd[3] = (vUShPort[0]>>8 & 0xff);
        vCmd[4] = (vUShPort[0] & 0xff);

        //desination port number
        vCmd[5] =(vUShPort[1]>>8 & 0xff);
        vCmd[6] =(vUShPort[1] & 0xff);
        
        //desination port number
        vCmd[7] =(vUShPort[2]>>8 & 0xff);
        vCmd[8] =(vUShPort[2] & 0xff);
        
        m_objNetTCPInterface->SendData(vCmd);
    }

    void LambdaModule::MatrixFastClear()
    {
        LOG_TRACE(__FUNCTION__);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x02;
        m_objNetTCPInterface->SendData(vCmd);
    }

    vector<char> LambdaModule::ReadOMR(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
	
        // Create a vector to receive OMR data
        // NOTE - length of data can be variable - need to be careful - is it 36 or 40 bytes?
        // Reversing is dangerous...
        int32 lengthOMR = 36; // OMR is 36 bytes. Should ditch excess if possible.
        int32 lengthBuffer = 1024; // Oversized buffer to deal with excess data. 
        vector<char> vRecvOMR(lengthBuffer,0x00);
        vector<char> vOut; // Empty vector for output

        m_vStChips[nChipNo-1].vStrOMR[3] &= 0x1F; // AND byte with 0001 1111 to set first 3 bits
        m_vStChips[nChipNo-1].vStrOMR[3] |= 0xE0; // 0xE0 is used for read OMR (111)
        EnableChip(nChipNo);

        m_objNetTCPInterface->SendData(m_vStChips[nChipNo-1].vStrOMR);
        
        if(m_strSystemType == "handshake")
            int responseval = CheckResponse(); // Basic check of response rather than sleep
        
        m_objNetTCPInterface->SendData(m_vExeCmd);
        // Having executed command, need to receive data back over TCP interface
        // Note that we request at least 1 byte; total data is very short, so this means we want at least one packet but are agnostic about the length
        char * pRecvOMR = vRecvOMR.data();
        int nDataReceived = 0;
        
        szt length_min,length_max,total_bytes;

        length_min = lengthOMR;
        length_max = lengthBuffer;
        total_bytes = nDataReceived;
        
        m_objNetTCPInterface->ReceiveData(pRecvOMR,length_min,length_max,total_bytes);
        //m_objNetTCPInterface->ReceiveData(pRecvOMR,lengthOMR,lengthBuffer,nDataReceived);
        lengthOMR = length_min;
        lengthBuffer = length_max;
        nDataReceived = total_bytes;
        
        
        // Truncate to size of OMR
        vRecvOMR.resize(lengthOMR);
        // Reverse string to make it more understandable
        for(int32 i=(vRecvOMR.size()-1); i>=0; --i)
        {
            char charrev = StringUtils::ReverseChar(vRecvOMR[i]);
            vOut.push_back(charrev);
        }

        PrepNextImaging();
        
        return vOut;
    
    }

    string LambdaModule::GetChipID(int32 chip_no)
    {
        // Use Read OMR functionality to get the OMR value back.
        // This function waits for timeout when called.
        vector<char> v_OMRread = ReadOMR(chip_no);
        // Current version prints debug info when reading OMR -
        // this is sufficient for tests, but should implement some 
        // Medipix-specific decoding ultimately.
        // For now, just return arbitrary value
        string str_output = "";
        if(v_OMRread.size() < 14)
        {
            str_output = "Error";
        }
        else
        {
            string alphabet = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            uchar ycoord_raw = static_cast<uchar>(v_OMRread[13] & 0x0F); // Last 4 bits
            string ycoord = std::to_string(ycoord_raw);
            uchar xcoord_raw = static_cast<uchar>(v_OMRread[13] & 0xF0);
            xcoord_raw = xcoord_raw>>4;
            // Need to convert this to letter, where A=1, B=2 etc.
            // Possible danger? Remove minus 1 here temporarily?
            int32 xcoord_pos = (int32)xcoord_raw; 
            string xcoord = alphabet.substr(xcoord_pos,1);
            // Then, grab wafer number - need to be careful with signed vs unsigned char
            uchar lowerpart = static_cast<uchar>(v_OMRread[12]);
            uchar upperpart = static_cast<uchar>(v_OMRread[11] & 0x0F);
            uint32 waferval = lowerpart;
            waferval+= upperpart * 256;
            string waferstr = std::to_string(waferval);
            // Now, deal with possible corrections
            char correcttest = (v_OMRread[11] & 0x30)>>4;
            // Bit of a pain - construct 8 bit value spread across 2 bytes
            uchar correct_raw = static_cast<uchar>((v_OMRread[11] & 0xC0)>>6);
            correct_raw = correct_raw | ((v_OMRread[10] & 0x3F)<<2);
            switch(correcttest)
            {
                case 1: // Y correction
                    correct_raw = correct_raw & 0x0F;
                    ycoord = std::to_string(correct_raw);
                    break;
                case 2: // X correction
                    correct_raw = correct_raw & 0x0F;
                    xcoord_pos = static_cast<int32>(correct_raw);
                    xcoord = alphabet.substr(xcoord_pos,1);
                    break;
                case 3: // Wafer correction
                    lowerpart=correct_raw;
                    waferval = lowerpart;
                    waferval+= upperpart * 256;
                    waferstr = std::to_string(waferval);
                    break;
            }
            // Build output string
            str_output.append(waferstr);
            str_output.append(xcoord);
            str_output.append(ycoord);
        }
        return str_output;
    }

    int LambdaModule::CheckResponse()
    {
        LOG_TRACE(__FUNCTION__);
        
        // Call this after certain commands to (a) wait for detector reply and (b) check if command worked
        // Currently, ReceiveData command below has a very long timeout - in future may benefit from changing
        int lengthResponse = 40; // Response is 40 bytes
        int minlength = 30; // Wait for at least 30 bytes
        int lengthBuffer = 1600; // Oversized buffer to deal with excess data if problems occur. 
        vector<char> vRecv(lengthBuffer,0x00);
        char * pRecv = vRecv.data();
        szt nDataReceived = 0;
        int retval = 0;
        int nTotDataReceived = 0;
        int nmaxtries = 2000;
        int ntries = 0;

        // Will implement "timeout" in this code - aim for 1s. One cycle has 0.5ms timeout, so this is 2000.
        while((nTotDataReceived < minlength) && (ntries < nmaxtries))
        {
            retval = m_objNetTCPInterface->ReceivePacket(pRecv, nDataReceived);
            if(retval == 0) // Data present
            {
                nTotDataReceived+=nDataReceived;
                pRecv+=nDataReceived; // Move pointer along
            }
            else
            {
                ++ntries;
            }
        }      
	

        if(nTotDataReceived == 0)
        {
            std::cout << "Detector response timed out" << "\n";
            return -1;
        }
        else
        {
            retval = int(vRecv[0]);
            std::cout << "Detector response " << retval << "\n";
            return retval;
        }

        std::cout << "Shouldn't see this!" << "\n";
        return -1; // Shouldn't reach here, included for safety
    
    }  

    
    void LambdaModule::Reset()
    {
        LOG_TRACE(__FUNCTION__);
        
        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x01;
        //vCmd[2] = 0x00;
        m_objNetTCPInterface->SendData(vCmd);
    }


    void LambdaModule::SetupFastImaging()
    {
        LOG_TRACE(__FUNCTION__);

        SetAllDACs();
        ConfigAllMatrices();

        if(m_stDetCfg.bCRW && m_stDetCfg.nCounterMode == 3)
            WriteReadOutMode(1); // Special burst CRW for 1-chip system - only compatible with CRW mode of chip
        else if(m_stDetCfg.bCRW)
            WriteReadOutMode(3); // Standard CRW
        else if(m_stDetCfg.nCounterMode == 2)
            WriteReadOutMode(4); // Read out 2 counters e.g. in 24 bit
        else if(m_stDetCfg.nCounterMode ==0 ||m_stDetCfg.nCounterMode==1)
            WriteReadOutMode(2); // Read out 1 counter sequentially (0 or 1 - controlled by OMR)	
        PrepNextImaging();

    }

    void LambdaModule::StartFastImaging()
    {
        LOG_TRACE(__FUNCTION__);
        // In code update, will rely on PrepNextImaging to make sure
        // detector is ready for next acquisition. This makes things
        // more complicated (entanglement with state monitoring) but
        // hopefully improves performance.
        // Only send execute command to non-slave modules
        if(!m_bSlaveModule) m_objNetTCPInterface->SendData(m_vExeCmd); 
    }


    void LambdaModule::PrepNextImaging()
    {

        LOG_TRACE(__FUNCTION__);

        if(m_stDetCfg.shDataOutLines !=8)
        {
            m_stDetCfg.shDataOutLines = 8;
        }
                
        for(szt i=0;i<m_vCurrentUsedChips.size();i++)
            UpdateOMRString(m_vCurrentUsedChips[i]);

        EnableChip(0);                            
        // No of images and shutter time should already be set - don't need to send this.
        //According to the manual, To reset the full pixel counter
        //this command needs to do matrix fast clear twice
        MatrixFastClear();
        MatrixFastClear();

        m_vStChips[m_vCurrentUsedChips[0]-1].vStrOMR[3] &= 0x1F;

        if(m_stDetCfg.nCounterMode == 1)
            m_vStChips[m_vCurrentUsedChips[0]-1].vStrOMR[3] |=0x20;
        
        m_objNetTCPInterface->SendData(m_vStChips[m_vCurrentUsedChips[0]-1].vStrOMR);
	if(m_strSystemType == "handshake")
            int responseval = CheckResponse(); // Basic check of response rather than sleep
	
        // For slave modules, need to give execute command to ensure it's ready to roll
        if(m_bSlaveModule) m_objNetTCPInterface->SendData(m_vExeCmd);
    
    }


  
    void LambdaModule::StopFastImaging()
    {
        LOG_TRACE(__FUNCTION__);

        // When the fast imaging is done on Lambda, then this will simply send a signal to
        // module which will stop imaging.
        // In current implementation, do this by sending a command to take one image.
        //int32 lImgNoTmp = m_lImageNo;
        int16 shTriggerModeTmp = m_shTriggerMode;
        //double dShutterTimeTmp = m_dShutterTime;

        if(m_shTriggerMode>0)
            WriteTriggerMode(0);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
        vCmd[1] = 0xf0;
        vCmd[2] = 0x4a;
        m_objNetTCPInterface->SendData(vCmd);

        if(shTriggerModeTmp>0)
            WriteTriggerMode(shTriggerModeTmp);
    }
    
    void LambdaModule::SetDAC(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
            
        // OMR string byte[3] should be set to 100X XXXX (Set DAC code)
        // Do this by AND with 0001 1111 (0x1F) then OR with 1000 0000 (0x80)
        // Try long sleeps after sending longer data strings, to try to improve reliability
        m_vStChips[nChipNo-1].vStrOMR[3] &= 0x1F;
        m_vStChips[nChipNo-1].vStrOMR[3] |= 0x80;

        ///update DAC string
        UpdateDACString(nChipNo);
        //enable chip
        EnableChip(nChipNo);
            
        m_objNetTCPInterface->SendData(m_vStChips[nChipNo-1].vStrOMR);
	if(m_strSystemType == "handshake")
            int responseval = CheckResponse(); // Basic check of response rather than sleep
	
        m_objNetTCPInterface->SendData(m_vStChips[nChipNo-1].vStrDAC);

        if((m_strSystemType == "small") || (m_strSystemType == "handshake"))
            int responseval = CheckResponse(); // Basic check of response rather than sleep
    }

    void LambdaModule::SetAllDACs()
    {
        LOG_TRACE(__FUNCTION__);

        for(szt i=0;i<m_vCurrentUsedChips.size();i++)
            SetDAC(m_vCurrentUsedChips[i]);
    }

    void LambdaModule::UpdateOMRString(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
        //reset to 0
        std::fill(m_vStChips[nChipNo-1].vStrOMR.begin(),m_vStChips[nChipNo-1].vStrOMR.end(),0x00);

        m_vStChips[nChipNo-1].vStrOMR[1] = 0x30;

        //byte 3 corresponds to B0 of OMR
        if(m_stDetCfg.bCRW)
            m_vStChips[nChipNo-1].vStrOMR[3] |= 0x10;
        if(m_stDetCfg.bPolarity)
            m_vStChips[nChipNo-1].vStrOMR[3] |= 0x08;

        //DataOut lines code as follows. Note silly reversal is needed
        // 1 line -> code 00 = 0
        // 2 lines -> code 10 = 2
        // 4 lines -> code 01 = 1
        // 8 lines -> code 11 = 3
        int16 shDataOutLinesCode = 0;

        if(m_stDetCfg.shDataOutLines == 2)
            shDataOutLinesCode = 2;
        if(m_stDetCfg.shDataOutLines == 4)
            shDataOutLinesCode = 1;
        if(m_stDetCfg.shDataOutLines == 8)
            shDataOutLinesCode =3;
        m_vStChips[nChipNo-1].vStrOMR[3] |= (shDataOutLinesCode<<1);

        //new in RX bit 7
        if(m_stDetCfg.bDiscSPMCSM)
            m_vStChips[nChipNo-1].vStrOMR[3] |= 0x01;
        if(m_stDetCfg.bEnableTP)
            m_vStChips[nChipNo-1].vStrOMR[4] |= 0x80;

        // We want to deal with Counter depth carefully!
        // Default will be 12. We want to translate from this to the module codes as follows:
        // 1 bit -> 0
        // 4 bit -> 2
        // 12 bit -> 1
        // 24 bit -> 3

        int16 shCounterDepthCode = 1;
        if(m_stDetCfg.shCounterBitDepth == 1)
            shCounterDepthCode = 0;
        if(m_stDetCfg.shCounterBitDepth == 6)
            shCounterDepthCode = 2;
        if(m_stDetCfg.shCounterBitDepth == 24)
            shCounterDepthCode = 3;
        m_vStChips[nChipNo-1].vStrOMR[4] |=(shCounterDepthCode<<5);
            
        if(m_stDetCfg.shRowBlockSel > 0)
        {
            m_vStChips[nChipNo-1].vStrOMR[5] |= 0x20;
            m_vStChips[nChipNo-1].vStrOMR[4] |= (m_stDetCfg.shRowBlockSel>>2);
            m_vStChips[nChipNo-1].vStrOMR[5] |= (m_stDetCfg.shRowBlockSel<<6);
        }

        if(m_stDetCfg.bEqualise)
            m_vStChips[nChipNo-1].vStrOMR[5] |= 0x10;
        if(m_stDetCfg.bColorMode)
            m_vStChips[nChipNo-1].vStrOMR[5] |= 0x08;
        if(m_stDetCfg.bChargeSum)
            m_vStChips[nChipNo-1].vStrOMR[5] |= 0x04;
        if(m_stDetCfg.bOMRHeader)
            m_vStChips[nChipNo-1].vStrOMR[5] |=0x02;

        // Also need to include gainMode! These are bits 35 and 36, which belong to byte 7 (bits 3 and 4)
        // 00 - Super high gain
        // 10 - High gain
        // 01 - Low gain
        // 11 - Super low gain
        // Want to effectively decode this as 0 to 3 in turn to avoid oddities
        int16 shGainDepthCode = 0;

        if(m_stDetCfg.shGainMode == 1)
            shGainDepthCode = 2;

        if(m_stDetCfg.shGainMode == 2)
            shGainDepthCode = 1;
        
        if(m_stDetCfg.shGainMode == 3)
            shGainDepthCode = 3;

        m_vStChips[nChipNo-1].vStrOMR[7] |= (shGainDepthCode<<3);

        int16 shSenseDAC = FindDACCode(m_vStChips[nChipNo-1].strSenseDAC); //strSenseDAC
        int16 shExtDAC = FindDACCode(m_vStChips[nChipNo-1].strExtDAC); // strExtDAC

        m_vStChips[nChipNo-1].vStrOMR[7] |= (shSenseDAC>>2);
        m_vStChips[nChipNo-1].vStrOMR[8] |= (shSenseDAC<<6);
        m_vStChips[nChipNo-1].vStrOMR[8] |= (shExtDAC<<1);            
    }
        
    void LambdaModule::UpdateDACString(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
        //reset to 0
        std::fill(m_vStChips[nChipNo-1].vStrDAC.begin(),m_vStChips[nChipNo-1].vStrDAC.end(),0x00);
                        
        m_vStChips[nChipNo-1].vStrDAC[0] = 0xa0;
        m_vStChips[nChipNo-1].vStrDAC[1] = 0x00;

        m_vStChips[nChipNo-1].vStrDAC[2]|= m_vStChips[nChipNo-1].shTPREFA>>1;
        m_vStChips[nChipNo-1].vStrDAC[3]|= m_vStChips[nChipNo-1].shTPREFA<<7;

        m_vStChips[nChipNo-1].vStrDAC[3]|= m_vStChips[nChipNo-1].shTPREFB>>2;
        m_vStChips[nChipNo-1].vStrDAC[4]|= m_vStChips[nChipNo-1].shTPREFB<<6;

        m_vStChips[nChipNo-1].vStrDAC[4]|= m_vStChips[nChipNo-1].shCAS>>2;
        m_vStChips[nChipNo-1].vStrDAC[5]|= m_vStChips[nChipNo-1].shCAS<<6; 

        m_vStChips[nChipNo-1].vStrDAC[5]|= m_vStChips[nChipNo-1].shFBK>>2;
        m_vStChips[nChipNo-1].vStrDAC[6]|= m_vStChips[nChipNo-1].shFBK<<6;

        m_vStChips[nChipNo-1].vStrDAC[6]|= m_vStChips[nChipNo-1].shTPREF>>2;
        m_vStChips[nChipNo-1].vStrDAC[7]|= m_vStChips[nChipNo-1].shTPREF<<6;

        m_vStChips[nChipNo-1].vStrDAC[7]|= m_vStChips[nChipNo-1].shGND>>2;
        m_vStChips[nChipNo-1].vStrDAC[8]|= m_vStChips[nChipNo-1].shGND<<6;

        m_vStChips[nChipNo-1].vStrDAC[8]|= m_vStChips[nChipNo-1].shRPZ>>2;
        m_vStChips[nChipNo-1].vStrDAC[9]|= m_vStChips[nChipNo-1].shRPZ<<6;

        m_vStChips[nChipNo-1].vStrDAC[9] |= m_vStChips[nChipNo-1].shTPBufferIn>>2;
        m_vStChips[nChipNo-1].vStrDAC[10]|= m_vStChips[nChipNo-1].shTPBufferIn<<6;

        m_vStChips[nChipNo-1].vStrDAC[10]|= m_vStChips[nChipNo-1].shTPBufferOut>>2;
        m_vStChips[nChipNo-1].vStrDAC[11]|= m_vStChips[nChipNo-1].shTPBufferOut<<6;

        m_vStChips[nChipNo-1].vStrDAC[11]|= m_vStChips[nChipNo-1].shDelay>>2;
        m_vStChips[nChipNo-1].vStrDAC[12]|= m_vStChips[nChipNo-1].shDelay<<6;
            
        m_vStChips[nChipNo-1].vStrDAC[12]|= m_vStChips[nChipNo-1].shDACDiscH>>2;
        m_vStChips[nChipNo-1].vStrDAC[13]|= m_vStChips[nChipNo-1].shDACDiscH<<6;
            
        m_vStChips[nChipNo-1].vStrDAC[14]|= m_vStChips[nChipNo-1].shDACDiscL>>2;
        m_vStChips[nChipNo-1].vStrDAC[15]|= m_vStChips[nChipNo-1].shDACDiscL<<6;

        m_vStChips[nChipNo-1].vStrDAC[16]|= m_vStChips[nChipNo-1].shDiscLS>>2;
        m_vStChips[nChipNo-1].vStrDAC[17]|= m_vStChips[nChipNo-1].shDiscLS<<6;

        m_vStChips[nChipNo-1].vStrDAC[17]|= m_vStChips[nChipNo-1].shDisc>>2;
        m_vStChips[nChipNo-1].vStrDAC[18]|= m_vStChips[nChipNo-1].shDisc<<6;

        m_vStChips[nChipNo-1].vStrDAC[18]|= m_vStChips[nChipNo-1].shShaper>>2;
        m_vStChips[nChipNo-1].vStrDAC[19]|= m_vStChips[nChipNo-1].shShaper<<6;

        m_vStChips[nChipNo-1].vStrDAC[19]|= m_vStChips[nChipNo-1].shIkrum>>2;
        m_vStChips[nChipNo-1].vStrDAC[20]|= m_vStChips[nChipNo-1].shIkrum<<6;

        m_vStChips[nChipNo-1].vStrDAC[20]|= m_vStChips[nChipNo-1].shPreamp>>2;
        m_vStChips[nChipNo-1].vStrDAC[21]|= m_vStChips[nChipNo-1].shPreamp<<6;

        m_vStChips[nChipNo-1].vStrDAC[21]|= m_vStChips[nChipNo-1].vThreshold[7]>>3;
        m_vStChips[nChipNo-1].vStrDAC[22]|= m_vStChips[nChipNo-1].vThreshold[7]<<5;

        m_vStChips[nChipNo-1].vStrDAC[22]|= m_vStChips[nChipNo-1].vThreshold[6]>>4;
        m_vStChips[nChipNo-1].vStrDAC[23]|= m_vStChips[nChipNo-1].vThreshold[6]<<4;

        m_vStChips[nChipNo-1].vStrDAC[23]|= m_vStChips[nChipNo-1].vThreshold[5]>>5;
        m_vStChips[nChipNo-1].vStrDAC[24]|= m_vStChips[nChipNo-1].vThreshold[5]<<3;

        m_vStChips[nChipNo-1].vStrDAC[24]|= m_vStChips[nChipNo-1].vThreshold[4]>>6;
        m_vStChips[nChipNo-1].vStrDAC[25]|= m_vStChips[nChipNo-1].vThreshold[4]<<2;

        m_vStChips[nChipNo-1].vStrDAC[25]|= m_vStChips[nChipNo-1].vThreshold[3]>>7;
        m_vStChips[nChipNo-1].vStrDAC[26]|= m_vStChips[nChipNo-1].vThreshold[3]<<1;

        m_vStChips[nChipNo-1].vStrDAC[26]|= m_vStChips[nChipNo-1].vThreshold[2]>>8;
        m_vStChips[nChipNo-1].vStrDAC[27]|= m_vStChips[nChipNo-1].vThreshold[2];

        m_vStChips[nChipNo-1].vStrDAC[28]|= m_vStChips[nChipNo-1].vThreshold[1]>>1;
        m_vStChips[nChipNo-1].vStrDAC[29]|= m_vStChips[nChipNo-1].vThreshold[1]<<7;

        m_vStChips[nChipNo-1].vStrDAC[29]|= m_vStChips[nChipNo-1].vThreshold[0]>>2;
        m_vStChips[nChipNo-1].vStrDAC[30]|= m_vStChips[nChipNo-1].vThreshold[0]<<6;            
    }
        
    void LambdaModule::UpdateConfigStrings(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
        CreateMatrixConfig1(nChipNo);
    }

    void LambdaModule::ConfigAllMatrices()
    {
        LOG_TRACE(__FUNCTION__);
        for(szt i=0;i<m_vCurrentUsedChips.size();i++)
            LoadAndCheckMatrixConfig(m_vCurrentUsedChips[i]);
    }

    void LambdaModule::LoadAndCheckMatrixConfig(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
        UpdateConfigStrings(nChipNo);
        LoadMatrixConfigRXHack(nChipNo,m_vStChips[nChipNo-1].vStrConfig1,1);       
    }
    
    bool LambdaModule::LoadMatrixConfigRXHack(int32 nChipNo,
                                              vector<uchar> vStrConfigData,
                                              int32 nCounterNo)
    {
        LOG_TRACE(__FUNCTION__);
            
        // Due to a bug in Medipix3RX, the config loading process becomes more convoluted
        // In effect, we do two loads, with RowBlock set to 111 (7) so that half the matrix
        // is loaded at a time.
        // NOTE that the firmware expects the loaded data to be as long as the matrix!
        // So, to get this to work,
        // I need to pad each set of data out to the length of a full matrix.

        char chTemp;

	int responseval = 0;
        
        int16 shRowBlockSelTemp = m_stDetCfg.shRowBlockSel;
        m_stDetCfg.shRowBlockSel = 7;

        // Find length of half of config, in bytes
        int32 nHalfConfigLength = BYTES_IN_CHIP/2;
            
        UpdateOMRString(nChipNo);
        // Then work with matrix config
        string strSubConfigDataString;
        
        m_vStChips[nChipNo-1].vStrOMR[3] &= 0x1F;
        // Reset first 3 bits of OMRString[3] to zero
        if (nCounterNo == 1)
            m_vStChips[nChipNo-1].vStrOMR[3] |= 0x60;
        // Set first 3 bits to 011 to do counter 1
        else
            m_vStChips[nChipNo-1].vStrOMR[3] |= 0x40;

        // Set first 3 bits to 010 to do counter 0
        // Send first half of config data.
        // First byte of this data string is set to 1010 - the "execute" code
        chTemp = 0xa0;
        strSubConfigDataString.append(1,chTemp);
            
        // Then the empty presync byte
        chTemp = 0x00;
        strSubConfigDataString.append(1,chTemp);

        // Then the data string itself is appended - HERE, WE ONLY USE HALF LENGTH!!!!
        strSubConfigDataString.append(vStrConfigData.begin(),
                                      vStrConfigData.begin()+nHalfConfigLength);

        // Then pad out to correct length - maybe plus 36 (see manual)
        strSubConfigDataString.resize(vStrConfigData.size()+2);

        EnableChip(nChipNo);
        m_objNetTCPInterface->SendData(m_vStChips[nChipNo-1].vStrOMR);
        if(m_strSystemType == "handshake")
            int responseval = CheckResponse(); // Basic check of response rather than sleep
	
        m_objNetTCPInterface->SendData(const_cast<char*>(strSubConfigDataString.c_str()),
                                       strSubConfigDataString.size());
        // NEWSINGLECHIP BUG FIX CODE
        if((m_strSystemType == "small") || (m_strSystemType == "handshake"))
            int responseval = CheckResponse(); // Basic check of response rather than sleep
            
        strSubConfigDataString.clear();
        // First byte of this data string is set to 1010 - the "execute" code
        chTemp = 0xa0;
        strSubConfigDataString.append(1,chTemp);
        chTemp = 0x00;
        // Then the empty presync byte
        strSubConfigDataString.append(1,chTemp);

            
        strSubConfigDataString.append(vStrConfigData.begin()+nHalfConfigLength,
                                      vStrConfigData.begin()+2*nHalfConfigLength);
        // Then the data string itself is appended - HERE, WE ONLY USE HALF LENGTH!!!!

        strSubConfigDataString.resize(vStrConfigData.size()+2);
        // Then pad out to correct length - need to check this!
        // Don't know why Michael's code fiddles with original config data string!

        EnableChip(nChipNo);
        m_objNetTCPInterface->SendData(m_vStChips[nChipNo-1].vStrOMR);
        if(m_strSystemType == "handshake")
            int responseval = CheckResponse(); // Basic check of response rather than sleep
	
        m_objNetTCPInterface->SendData(const_cast<char*>(strSubConfigDataString.c_str()),
                                       strSubConfigDataString.size());
        if((m_strSystemType == "small") || (m_strSystemType == "handshake"))
            int responseval = CheckResponse(); // Basic check of response rather than sleep

        // Set back rowBlockSel
        m_stDetCfg.shRowBlockSel = shRowBlockSelTemp;
        UpdateOMRString(nChipNo);
        return true;        
    }

    void LambdaModule::CreateMatrixConfig1(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);
            
        // Initialise variables needed
        int32 nRow = 0; //Row of pixels we're currently working with
        int32 nConfigBit = 0; //Bit of config register (for given row) we're working on
        int32 nPixel = 0; //Pixel we're working with
        int32 nOutByte = 0; //Output byte we're currently working with
        int32 nOutBit = 0; //Output bit we're currently working with

        // Global variable CHIPSIZE sets dimensions of chip
        // Bit depth is automatically 12 for this operation!
        int32 nBitsPerCounter = 12;
            
        uchar chBitValue;
            
        // The output will be in bytes. I will assume that the lowest-value bit in
        // each byte is "bit zero". If we run into problems, I should be able to
        // reverse the bytes fairly easily anyway.
        // The "pixel 1" configuration register works as follows FOR MEDIPIX# RX:
        // 0 - maskBit
        // 1 - configTHA - bit 0
        // 2 - configTHA - bit 1
        // 3 - configTHA - bit 2
        // 4 - configTHA - bit 3
        // 5 - configTHA - bit 4
        // 6 - configTHB - bit 0
        // 7 - configTHB - bit 1
        // 8 - configTHB - bit 2
        // 9 - configTHB - bit 3
        // 10 - configTHB - bit 4
        // 11 - testBit

        // Already have variables for relevant chip as Vectors:
        // testBit, gainMode, maskBit, configTHA, configTHB, config0String, config1String
        // Loop over rows
        for( nRow = 0; nRow < CHIP_SIZE; nRow++ )
        {
            // Loop over config bit from 11 to 0
            for( nConfigBit = nBitsPerCounter-1 ; nConfigBit >= 0; nConfigBit-- )
            {
                for( nPixel = 0; nPixel < CHIP_SIZE; nPixel++ )
                {
                    // First, need to work out the value of the output bit, using a CASE statement
                    switch (nConfigBit)
                    {
                        case 0: 
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vMaskBit.at(
                                                     nPixel + CHIP_SIZE * nRow), 0);
                            break;
                        case 1: 
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHA.at(
                                                     nPixel + CHIP_SIZE * nRow), 0);
                            break;
                        case 2:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHA.at(
                                                     nPixel + CHIP_SIZE * nRow), 1);
                            break;
                        case 3:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHA.at(
                                                     nPixel + CHIP_SIZE * nRow), 2);
                            break;
                        case 4:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHA.at(
                                                     nPixel + CHIP_SIZE * nRow), 3);
                            break;
                        case 5:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHA.at(
                                                     nPixel + CHIP_SIZE * nRow), 4);
                            break;
                        case 6:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHB.at(
                                                     nPixel + CHIP_SIZE * nRow), 0);
                            break;
                        case 7:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHB.at(
                                                     nPixel + CHIP_SIZE * nRow), 1);
                            break;
                        case 8:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHB.at(
                                                     nPixel + CHIP_SIZE * nRow), 2);
                            break;
                        case 9:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHB.at(
                                                     nPixel + CHIP_SIZE * nRow), 3);
                            break;
                        case 10:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vConfigTHB.at(
                                                     nPixel + CHIP_SIZE * nRow), 4);
                            break;
                        case 11:
                            chBitValue = Grabbit(m_vStChips[nChipNo-1].vTestBit.at(
                                                     nPixel + CHIP_SIZE * nRow), 0);
                            break;
                        default:
                            chBitValue = 0;
                            break;
                    }
                    // Then, update config vector
                    // Do this by looking at relevant byte, shifting it up
                    // (higher value) and inserting new low bit by bitwise OR
                    m_vStChips[nChipNo-1].vStrConfig1[nOutByte]
                        = m_vStChips[nChipNo-1].vStrConfig1[nOutByte]<<1;
                        
                    m_vStChips[nChipNo-1].vStrConfig1[nOutByte] |=  chBitValue;
                    nOutBit++;

                    //Then, keep track of where we are so that we move onto
                    //next output byte at correct point
                    if (nOutBit > 7)
                    {
                        nOutByte++;
                        nOutBit = 0;
                    }
                }
            }
        }
    }
    
    uchar LambdaModule::Grabbit(int16 shData, int32 nBit)
    {
        //LOG_TRACE(__FUNCTION__);
        uchar chOutputByte;
        chOutputByte  = shData >> nBit;
        chOutputByte &= 0x01;
        return chOutputByte;           
    }

    void LambdaModule::EnableChip(int32 nChipNo)
    {
        LOG_TRACE(__FUNCTION__);

        vector<uchar> vCmd(COMMAND_LENGTH,0x00);
            
        switch(nChipNo)
        {
            case 0:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x32;
                vCmd[3] = 0x00;
                break;
            case 1:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x33;
                vCmd[3] = 0x00;
                break;
            case 2:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x34;
                vCmd[3] = 0x00;
                break;
            case 3:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x35;
                vCmd[3] = 0x00;
                break;
            case 4:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x36;
                vCmd[3] = 0x00;
                break;
            case 5:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x37;
                vCmd[3] = 0x00;
                break;
            case 6:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x38;
                vCmd[3] = 0x00;
                break;
            case 7:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x39;
                vCmd[3] = 0x00;
                break;
            case 8:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x3A;
                vCmd[3] = 0x00;
                break;
            case 9:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x3B;
                vCmd[3] = 0x00;
                break;
            case 10:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x3C;
                vCmd[3] = 0x00;
                break;
            case 11:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x3D;
                vCmd[3] = 0x00;
                break;
            case 12:
                vCmd[1] = 0xf0;
                vCmd[2] = 0x3E;
                vCmd[3] = 0x00;
                break;
            default:
                LOG_STREAM(__FUNCTION__,ERROR,"chip No. is wrong");
                break;
        }
        //        char* chTemp = reinterpret_cast<char*>(&vCmd[0]);
        m_objNetTCPInterface->SendData(vCmd);
    }

    int16 LambdaModule::FindDACCode(string vStrDACName)
    {
        LOG_TRACE(__FUNCTION__);
        // Given an input string, we return a int16 corresponding to
        // the 5-bit code needed for the OMR
        // Default is zero. Also, we should have an empty string or
        // 'none' as options
        int16 shDACCode = 0;
        if (vStrDACName == "")
            shDACCode = 0; // Nothing!
        else if (vStrDACName == "none")
            shDACCode = 0; // Nothing!
        else if (vStrDACName == "threshold0")
            shDACCode = 16; // Manual 00001, reversed to 10000 = 16
        else if (vStrDACName == "threshold1")
            shDACCode = 8; // Manual 00010, reversed to 01000 = 8
        else if (vStrDACName == "threshold2")
            shDACCode = 24; // Manual 00011, reversed to 11000 = 24
        else if (vStrDACName == "threshold3")
            shDACCode = 4; // Manual 00100, reversed to 00100 = 4
        else if (vStrDACName == "threshold4")
            shDACCode = 20; // Manual 00101, reversed to 10100 = 20
        else if (vStrDACName == "threshold5")
            shDACCode = 12; // Manual 00110, reversed to 01100 = 12
        else if (vStrDACName == "threshold6")
            shDACCode = 28; // Manual 00111, reversed to 11100 = 28
        else if (vStrDACName == "threshold7")
            shDACCode = 2; // Manual 01000, reversed to 00010 = 2
        else if (vStrDACName == "preamp")
            shDACCode = 18; // Manual 01001, reversed to 10010 =  18
        else if (vStrDACName == "ikrum")
            shDACCode = 10; // Manual 01010, reversed to 01010 = 10
        else if (vStrDACName == "shaper")
            shDACCode = 26; // Manual 01011, reversed to 11010 = 26
        else if (vStrDACName == "disc")
            shDACCode = 6; // Manual 01100, reversed to 00110 = 6
        else if (vStrDACName == "disc_LS")
            shDACCode = 22; // Manual 01101, reversed to 10110 = 22
        // Manual 01110, reversed to 01110 = 14
        //else if (vStrDACName == "thresholdN") shDACCode = 14;
        //// Manual 01111, reversed to 11110 = 30
        //else if (vStrDACName == "DAC_pixel") shDACCode = 30; 
        else if (vStrDACName == "DAC_discL")
            shDACCode = 30; // Manual 01111, reversed to 11110 = 30
        else if (vStrDACName == "DAC_discH")
            shDACCode = 31; // Manual 11111, reversed to 11111 = 31
        else if (vStrDACName == "delay")
            shDACCode = 1; // Manual 10000, reversed to 00001 = 1
        else if (vStrDACName == "TP_BufferIn")
            shDACCode = 17; // Manual 10001, reversed to 10001 = 17
        else if (vStrDACName == "TP_BufferOut")
            shDACCode = 9; // Manual 10010, reversed to 01001 = 9
        else if (vStrDACName == "RPZ")
            shDACCode = 25; // Manual 10011, reversed to 11001 = 25
        else if (vStrDACName == "GND")
            shDACCode = 5; // Manual 10100, reversed to 00101 = 5
        else if (vStrDACName == "TP_REF")
            shDACCode = 21; // Manual 10101, reversed to 10101 = 21
        else if (vStrDACName == "FBK")
            shDACCode = 13; // Manual 10110, reversed to 01101 = 13
        else if (vStrDACName == "CAS")
            shDACCode = 29; // Manual 10111, reversed to 11101 = 29
        else if (vStrDACName == "TP_REFA")
            shDACCode = 3; // Manual 11000, reversed to 00011 = 3
        else if (vStrDACName == "TP_REFB")
            shDACCode = 19; // Manual 11001, reversed to 10011 = 19
        // Then we have a few additional codes for the sense
        else if (vStrDACName == "bandgap")
            shDACCode = 11; // Manual 11010, reversed to 01011 = 11
        else if (vStrDACName == "temp")
            shDACCode = 27; // Manual 11011, reversed to 11011 = 27
        else if (vStrDACName == "DACbias")
            shDACCode = 7; // Manual 11100, reversed to 00111 = 7
        else if (vStrDACName == "DACcascodebias")
            shDACCode = 23; // Manual 11101, reversed to 10111 = 23
        return shDACCode;
    }
}

