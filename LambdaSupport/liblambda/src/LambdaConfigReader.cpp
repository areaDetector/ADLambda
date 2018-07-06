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

#include "LambdaConfigReader.h"

namespace DetLambdaNS
{

    LambdaConfigReader::LambdaConfigReader()
    {
        LOG_TRACE(__FUNCTION__);
    }

    LambdaConfigReader::LambdaConfigReader(string _strConfigFilePath)
        :m_strConfigFilePath(_strConfigFilePath)
        ,m_strSystemConfigFile(SYSTEM_CONFIG_FILE)
        ,m_strDetConfigFile(DETECTOR_CONFIG_FILE)
        ,m_vStrMAC(2,vector<string>(2,"00:00:00:00:00:00"))
        ,m_vStrIP(2,vector<string>(2,"127.0.0.1"))
        ,m_vUShPort(3,0)
        ,m_vfTranslation(3,-1.0)
        ,m_nX(-1)
        ,m_nY(-1)
        ,m_nMaxRawImageNumbers(RAW_BUFFER_LENGTH)
        ,m_nMaxDecodedImageNumbers(DECODED_BUFFER_LENGTH)
        ,m_bSlaveModule(false)
        ,m_strSystemType("standard")
        ,m_nChunkIn(DEFAULT_COMPRESSION_CHUNK)
        ,m_nChunkOut(DEFAULT_COMPRESSION_CHUNK)
        ,m_nPostCode(0)
    {
        LOG_TRACE(__FUNCTION__);

        m_bMultilink = MULTI_LINK;
        m_bBurstMode = true;

        //TCP link config
        m_strTCPIPAddress = TCP_CONTROL_IP_ADDRESS;
        m_shTCPPortNo = TCP_CONTROL_PORT;

        //destination IP for CH0
        m_vStrIP[0][0] = UDP_CONTROL_IP_ADDRESS;
        //destination IP for CH1
        m_vStrIP[1][0] = UDP_CONTROL_IP_ADDRESS_1;

        //destination port No for CH0
        m_vUShPort[1] = UDP_PORT;
        m_vUShPort[2] = UDP_PORT_1;

        m_vNDecodingThreads.resize(3,2);
        // Decoding threads with HIGH priority
        m_vNDecodingThreads[0] = 3;
        // Threads with moderate NORMAL priority - run at lower frame rates
        m_vNDecodingThreads[1] = 3;
        // Threads with LOW priority that only run after all images received
        m_vNDecodingThreads[2] = 6;
        // Shutter time below which we should use fewer decoding threads while receiving
        m_dCriticalShutterTime = 2.1;


        m_objFileOp = new FileReader(m_strConfigFilePath);
        m_objStrUtil = new StringUtils();
    }

    LambdaConfigReader::~LambdaConfigReader()
    {
        LOG_TRACE(__FUNCTION__);

        m_vStrMAC.clear();
        m_vStrIP.clear();
        m_vUShPort.clear();

        m_vCurrentChip.clear();
        m_vStCurrentChipData.clear();

        delete m_objFileOp;
        m_objFileOp = NULL;

        m_vUnPixelMask.clear();
        m_vNIndex.clear();
        m_vNNominator.clear();
    }


    void LambdaConfigReader::GetUDPConfig(vector<vector<string>>& vStrMAC,
                                          vector<vector<string>>& vStrIP,
                                          vector<uint16>& vUShPort)
    {
        LOG_TRACE(__FUNCTION__);

        vStrMAC = m_vStrMAC;
        vStrIP = m_vStrIP;
        vUShPort = m_vUShPort;
    }

    void LambdaConfigReader::GetTCPConfig(string& strTCPIPAddr,int16& shTCPPort)
    {
        LOG_TRACE(__FUNCTION__);

        strTCPIPAddr = m_strTCPIPAddress;
        shTCPPort = m_shTCPPortNo;
    }

    bool LambdaConfigReader::GetMultilink() const
    {
        LOG_TRACE(__FUNCTION__);

        return m_bMultilink;
    }

    bool LambdaConfigReader::GetBurstMode() const
    {
        LOG_TRACE(__FUNCTION__);

        return m_bBurstMode;
    }

    void LambdaConfigReader::GetChipConfig(vector<int16>& vCurrentUsedChips,
                                           vector<stMedipixChipData>& vStCurrentChipData)
    {
        LOG_TRACE(__FUNCTION__);

        vCurrentUsedChips = m_vCurrentChip;
        vStCurrentChipData = m_vStCurrentChipData;
    }

    string LambdaConfigReader::GetOperationMode()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strOperationMode;
    }

    string LambdaConfigReader::GetModuleName()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strCurrentModuleName;
    }

    stDetCfgData LambdaConfigReader::GetDetConfigData()
    {
        LOG_TRACE(__FUNCTION__);

        return m_stDetCfgData;
    }

    string LambdaConfigReader::GetSystemType()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strSystemType;
    }

    void LambdaConfigReader::LoadLocalConfig(bool bOpModeRunningSwitch,string strOpMode)
    {
        LOG_TRACE(__FUNCTION__);

        //clear data in vector
        m_vCurrentChip.clear();
        m_vStCurrentChipData.clear();
        m_vStCurrentChipData.resize(STANDARD_CHIP_NUMBERS);

        //detector is already running.
        //in this case, the system config is not loaded again when operation mode is switched.
        if(!bOpModeRunningSwitch)
        {
            //read system config file
            GetDataFromSystemConfig();
        }
        else
            m_strOperationMode = strOpMode;

        //read det config file
        GetDataFromDetConfig();

        //read data from chip config file
        GetDataFromChipConfig();

        //read data from lookup folders for distortion correction
        GetDataFromLookupFolders();
    }

    vector<uint32> LambdaConfigReader::GetPixelMask()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vUnPixelMask;
    }

    vector<int32> LambdaConfigReader::GetIndexFile()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vNIndex;
    }

    vector<int32> LambdaConfigReader::GetNominatorFile()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vNNominator;
    }

    vector<float> LambdaConfigReader::GetPosition()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vfTranslation;
    }


    void LambdaConfigReader::GetDistoredImageSize(int32& nX,int32& nY)
    {
        LOG_TRACE(__FUNCTION__);

        nX = m_nX;
        nY = m_nY;
    }

    int32 LambdaConfigReader::GetRawBufferLength()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nMaxRawImageNumbers;
    }

    int32 LambdaConfigReader::GetDecodedBufferLength()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nMaxDecodedImageNumbers;
    }

    vector<int32> LambdaConfigReader::GetDecodingThreadNumbers()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vNDecodingThreads;
    }

    double LambdaConfigReader::GetCriticalShutterTime()
    {
        LOG_TRACE(__FUNCTION__);

        return m_dCriticalShutterTime;
    }

    bool LambdaConfigReader::GetSlaveModule()
    {
        LOG_TRACE(__FUNCTION__);

        return m_bSlaveModule;
    }

    void LambdaConfigReader::GetCompressionChunkConfig(int& nChunkIn,int& nChunkOut,int& nPostCode)
    {
        nChunkIn = m_nChunkIn;
        nChunkOut = m_nChunkOut;
        nPostCode = m_nPostCode;
    }

    //////////////////////////////////////////////////
    ///Private Methods
    //////////////////////////////////////////////////
    void LambdaConfigReader::GetDataFromLookupFolders()
    {
        LOG_TRACE(__FUNCTION__);

        string strFullPath = m_strConfigFilePath
            +"/"
            +m_strCurrentModuleName
            +"/lookups";

        string strDelimiter = "_";
        vector<string> vStrSplittedData;

        //get file names in the directory
        FileUtils objFu;
        vector<string> vStrFiles = objFu.GetFileList(strFullPath);

        string strIndexPrefix = "correction";
        string strNominatorPrefix = "nominator";
        string strPixelMaskPrefix = "pixelmask";

        // if(vStrFiles.size()!=3 || vStrFiles.empty())
        //     LOG_STREAM(__FUNCTION__,ERROR,"File Numbers are not correct");

        if(vStrFiles.empty())
            LOG_STREAM(__FUNCTION__,ERROR,"There is no calibration file");

        for(szt i=0;i<vStrFiles.size();i++)
        {
            string strPathTmp=string("");

            vector<int32> vNData;

            string strTmp = vStrFiles[i];
            //cout<<strTmp.substr(strTmp.length()-4,3)<<endl;

            //ignore nexus file
            if(strTmp.find("nxs") == std::string::npos)
            {
                if(!vStrSplittedData.empty())
                    vStrSplittedData.clear();

                m_objStrUtil->StrSplit(strTmp,strDelimiter,vStrSplittedData);

                strPathTmp = strFullPath + "/" + strTmp;

                //if file name cannot be splitted to correct size, skip
                if(vStrSplittedData.size() < 4)
                    continue;

                //get data from binary file
                if(m_objFileOp->FileExists(strPathTmp))
                {
                    m_objFileOp->SetFilePath(strPathTmp);
                    //read from .bin
                    m_objFileOp->OpenFile(true);
                    vNData = m_objFileOp->ReadDataFromIntBinaryFile();
                    m_objFileOp->CloseFile();
                }

                //check which file is actually read
                if(vStrSplittedData[0] == strIndexPrefix)
                    m_vNIndex.assign(vNData.begin(),vNData.end());
                else if(vStrSplittedData[0] == strNominatorPrefix)
                    m_vNNominator.assign(vNData.begin(),vNData.end());
                else if(vStrSplittedData[0] == strPixelMaskPrefix)
                    m_vUnPixelMask.assign(vNData.begin(),vNData.end());

                m_nX = atoi(vStrSplittedData[2].c_str());
                m_nY = atoi(vStrSplittedData[3].c_str());

                if(vNData.size()!= static_cast<szt>(m_nX*m_nY))
                    LOG_STREAM(__FUNCTION__,ERROR,"File size is not correct");
            }
        }
    }

    void LambdaConfigReader::GetDataFromSystemConfig()
    {
        LOG_TRACE(__FUNCTION__);

        vector<string> vStrCfg;
        string strFullPath = m_strConfigFilePath+"/"+m_strSystemConfigFile;
        m_objFileOp->SetFilePath(strFullPath);

        //load system config file
        m_objFileOp->OpenFile(false);
        vStrCfg = m_objFileOp->ReadDataFromFile();
        m_objFileOp->CloseFile();
        ExtractDataFromSysConfig(vStrCfg);
        vStrCfg.clear();
    }

    void LambdaConfigReader::GetDataFromDetConfig()
    {
        LOG_TRACE(__FUNCTION__);

        //check if module name is empty
        if(m_strCurrentModuleName.empty())
            LOG_STREAM(__FUNCTION__,ERROR,
                       "cannot load detector config files, module name is empty!!!");
        else
        {
            vector<string> vStrCfg;
            //detector config file path
            string strFullPath = m_strConfigFilePath+"/"
                +m_strCurrentModuleName
                +"/"
                +m_strOperationMode
                +"/"
                +m_strDetConfigFile;
            m_objFileOp->SetFilePath(strFullPath);

            //load detector config file
            m_objFileOp->OpenFile(false);
            vStrCfg = m_objFileOp->ReadDataFromFile();
            m_objFileOp->CloseFile();
            ExtractDataFromDetConfig(vStrCfg);
            vStrCfg.clear();
        }
    }

    void LambdaConfigReader::GetDataFromChipConfig()
    {
        LOG_TRACE(__FUNCTION__);

        vector<string> vStrCfg;
        string strFullPath;
        //load config for specific chip
        //# can be the number of current used chips
        //* means A or B
        string strChipTxtFileFormat = string("Chip#_DACsetting.txt");
        string strChipBinFileFormat = string("Chip#_TH*setting.bin");
        string strChipMaskFileFormat = string("Chip#_mask.bin");
        string strTxtFileNameTemp;
        string strBinFileNameTemp1;
        string strBinFileNameTemp2;
        string strMaskFileNameTemp;
        //load configurations for each chip
        for(szt i=0;i<m_vCurrentChip.size();i++)
        {
            strTxtFileNameTemp = strChipTxtFileFormat;
            strBinFileNameTemp1 = strChipBinFileFormat;
            strBinFileNameTemp2 = strChipBinFileFormat;
            strMaskFileNameTemp = strChipMaskFileFormat;
            m_objStrUtil->FindAndReplace(strTxtFileNameTemp,"#",to_string((long long int)m_vCurrentChip[i]));
            strFullPath =  m_strConfigFilePath+"/"
                +m_strCurrentModuleName
                +"/"
                +m_strOperationMode
                +"/"
                +strTxtFileNameTemp;
            if(m_objFileOp->FileExists(strFullPath))
            {
                m_objFileOp->SetFilePath(strFullPath);

                //read data from Chip#_DACsetting.tx
                m_objFileOp->OpenFile(false);
                vStrCfg = m_objFileOp->ReadDataFromFile();
                m_objFileOp->CloseFile();
                ExtractDataFromChipConfig(vStrCfg,m_vCurrentChip[i]-1);
                vStrCfg.clear();
            }



            //read threshold adjust files (thresholds A and B)
            m_objStrUtil->FindAndReplace(strBinFileNameTemp1,"#",to_string((long long int)m_vCurrentChip[i]));
            m_objStrUtil->FindAndReplace(strBinFileNameTemp1,"*","A");
            strFullPath =  m_strConfigFilePath+"/"
                +m_strCurrentModuleName
                +"/"
                +m_strOperationMode
                +"/"
                +strBinFileNameTemp1;

            if(m_objFileOp->FileExists(strFullPath))
            {
                m_objFileOp->SetFilePath(strFullPath);

                //read from THAsetting.bin
                m_objFileOp->OpenFile(true);
                m_vStCurrentChipData[m_vCurrentChip[i]-1].vConfigTHA = m_objFileOp->ReadDataFromBinaryFile();
                m_objFileOp->CloseFile();
            }


            m_objStrUtil->FindAndReplace(strBinFileNameTemp2,"#",to_string((long long int)m_vCurrentChip[i]));
            m_objStrUtil->FindAndReplace(strBinFileNameTemp2,"*","B");
            strFullPath =  m_strConfigFilePath+"/"
                +m_strCurrentModuleName
                +"/"
                +m_strOperationMode
                +"/"
                +strBinFileNameTemp2;

            if(m_objFileOp->FileExists(strFullPath))
            {
                m_objFileOp->SetFilePath(strFullPath);
                //read from THBsetting.bin
                m_objFileOp->OpenFile(true);
                m_vStCurrentChipData[m_vCurrentChip[i]-1].vConfigTHB = m_objFileOp->ReadDataFromBinaryFile();
                m_objFileOp->CloseFile();
            }

            // Read pixel mask file - this will mask pixels in HARDWARE (i.e. pixel will return 0)
            m_objStrUtil->FindAndReplace(strMaskFileNameTemp,"#",to_string((long long int)m_vCurrentChip[i]));
            strFullPath =  m_strConfigFilePath+"/"
                +m_strCurrentModuleName
                +"/"
                +m_strOperationMode
                +"/"
                +strMaskFileNameTemp;

            if(m_objFileOp->FileExists(strFullPath))
            {
                m_objFileOp->SetFilePath(strFullPath);

                //read from THAsetting.bin
                m_objFileOp->OpenFile(true);
                m_vStCurrentChipData[m_vCurrentChip[i]-1].vMaskBit = m_objFileOp->ReadDataFromBinaryFile();
                m_objFileOp->CloseFile();
            }


        }
    }

    void  LambdaConfigReader::ExtractDataFromSysConfig(vector<string> vStrSysCfg)
    {
        LOG_TRACE(__FUNCTION__);

        if(vStrSysCfg.size()==0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"No data can be extract for system config!!!");
            return;
        }

        vector<string> vSplittedVal;
        string strDelimiter = " ";

        for(szt i=0;i<vStrSysCfg.size();i++)
        {
            if(!vSplittedVal.empty())
                vSplittedVal.clear();

            //split data
            //original data structure:
            //e.g. defineOperationMode TwentyFourBit 0 3 Twenty_four_bit_readout
            m_objStrUtil->StrSplit(vStrSysCfg[i],strDelimiter,vSplittedVal);

            for(szt j=0;j<vSplittedVal.size();j++)
            {
                if(vSplittedVal[0] == "defineOperationMode")
                {
                    ///TODO::
                    //
                    //defineOperationMode TwentyFourBit 0 3 Twenty_four_bit_readout
                    //vSplittedVal[1] operation mode name
                    //vSplittedVal[2] refers to CRW off/on(relevant for trigger mode)
                    //vSplittedVal[3] gives No of subimages (relevant for reading image data)
                    //vSplittedVal[4] is a description of what the mode does.
                    //In this part, we need to replace underscore with space

                    //string strOpName = vSplittedVal[1];
                    //m_stDetCfgData.bCRW = atoi(vSplittedVal[2].c_str());

                    //int nSubImgs = atoi(vSplittedVal[3].c_str());
                    //this->SetSubImgNo(nSubImgs);

                    //string strOpTextMode = vSplittedVal[4];
                    //FindAndReplace(strOpTextMode,"_"," ");
                }
                else if(vSplittedVal[0] == "modulesPresent")
                {
                    //vSplittedVal[1] module name
                    m_strCurrentModuleName = vSplittedVal[1];
                }
	        else if(vSplittedVal[0] == "systemType")
                {
                    //vSplittedVal[1] module name
                    m_strSystemType = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "modulesPosition")
                {
                    m_vfTranslation[0] = atof(vSplittedVal[1].c_str());
                    m_vfTranslation[1] = atof(vSplittedVal[2].c_str());
                    m_vfTranslation[2] = atof(vSplittedVal[3].c_str());
                }
                else if(vSplittedVal[0] == "setOperatingMode")
                {
                    //vSplittedVal[1] operation mode
                    m_strOperationMode = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "setTriggerMode")
                {
                    ///NOT NEEDED!!!
                    //vSplittedVal[1] trigger mode
                    //short shTriggerMode = atoi(vSplittedVal[1].c_str());
                    //this->SetTriggerMode(shTriggerMode);
                }
                else if(vSplittedVal[0] == "setMultilink")
                {
                    m_bMultilink = (atoi(vSplittedVal[1].c_str()) == 1);
                }
                else if(vSplittedVal[0] == "setSlaveModule")
                {
                    m_bSlaveModule = (atoi(vSplittedVal[1].c_str()) == 1);
                }
                else if(vSplittedVal[0] == "setBurstMode")
                {
                    m_bBurstMode = (atoi(vSplittedVal[1].c_str()) != 0);
                }
                else if(vSplittedVal[0] == "defineDstPortNo")
                {
                    m_vUShPort[1] = atoi(vSplittedVal[1].c_str());
                    m_vUShPort[2] = atoi(vSplittedVal[2].c_str());
                }
                else if(vSplittedVal[0] == "defineSrcPortNo")
                {
                    m_vUShPort[0] = atoi(vSplittedVal[1].c_str());
                }
                else if(vSplittedVal[0] == "defineDstMACAddressCH0")
                {
                    m_vStrMAC[0][0] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineDstIPAddressCH0")
                {
                    m_vStrIP[0][0] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineDstMACAddressCH1")
                {
                    m_vStrMAC[1][0] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineDstIPAddressCH1")
                {
                    m_vStrIP[1][0] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineSrcMACAddressCH0")
                {
                    m_vStrMAC[0][1] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineSrcIPAddressCH0")
                {
                    m_vStrIP[0][1] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineSrcMACAddressCH1")
                {
                    m_vStrMAC[1][1] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineSrcIPAddressCH1")
                {
                    m_vStrIP[1][1] = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineDetIPAddressTCP")
                {
                    m_strTCPIPAddress = vSplittedVal[1];
                }
                else if(vSplittedVal[0] == "defineRawBuffer")
                {
                    m_nMaxRawImageNumbers = atoi(vSplittedVal[1].c_str());
                }
                else if(vSplittedVal[0] == "defineDecodedBuffer")
                {
                    m_nMaxDecodedImageNumbers = atoi(vSplittedVal[1].c_str());
                }
                else if(vSplittedVal[0] == "defineDecodingThreads")
                {
                    // Allows optimisation of number of decoding threads running
                    // under different circumstances
                    // Aim - optimise speed and reliability when running on different computers
                    // Require correct number of parameters
                    if(vSplittedVal.size()==5)
                    {
                        // Decoders that always run
                        m_vNDecodingThreads[0] = atoi(vSplittedVal[1].c_str());
                        // Decoders that run IF shutter time above critical value
                        // (see below), or image reception finished
                        m_vNDecodingThreads[1] = atoi(vSplittedVal[2].c_str());
                        // Decoders that run ONLY when image reception finished
                        m_vNDecodingThreads[2] = atoi(vSplittedVal[3].c_str());
                        // Critical shutter time
                        m_dCriticalShutterTime = atof(vSplittedVal[4].c_str());
                    }
                    else
                    {
                        LOG_STREAM(__FUNCTION__,ERROR,
                                   "Wrong number of parameters for decoding threads!!!");
                    }
                }
                else if(vSplittedVal[0]=="defineChunkIn")
                {
                    m_nChunkIn = atoi(vSplittedVal[1].c_str());
                }
                else if(vSplittedVal[0]=="defineChunkOut")
                {
                    m_nChunkOut = atoi(vSplittedVal[1].c_str());
                }
                else if(vSplittedVal[0]=="definePostCode")
                {
                    m_nPostCode = atoi(vSplittedVal[1].c_str());
                }
            }
        }
    }

    void LambdaConfigReader::ExtractDataFromDetConfig(vector<string> vStrDetCfg)
    {
        LOG_TRACE(__FUNCTION__);

        if(vStrDetCfg.size()==0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"No data can be extract for detector config!!!");
            return;
        }

        vector<string> vSplittedVal;
        string strDelimiter = " ";

        //check each string in data
        for(szt i=0;i<vStrDetCfg.size();i++)
        {
            if(!vSplittedVal.empty())
                vSplittedVal.clear();

            m_objStrUtil->StrSplit(vStrDetCfg[i],strDelimiter,vSplittedVal);

            for(szt j=0;j<vSplittedVal.size();j++)
            {
                string strResult = vSplittedVal[0];

                // skip comments line
                if(strResult != "//")
                {
                    if(strResult == "chipsPresent")
                    {
                        for(szt k=1;k<vSplittedVal.size();k++)
                            m_vCurrentChip.push_back(atoi(vSplittedVal[k].c_str()));
                        break;
                    }
                    else if(strResult == "thresholdsToScan")
                    {
                        for(szt k=1;k<vSplittedVal.size();k++)
                            m_stDetCfgData.vThresholdScan[k-1] = atoi(vSplittedVal[k].c_str());
                        break;
                    }
                    else if(strResult == "CRW")
                    {
                        m_stDetCfgData.bCRW = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "polarity")
                    {
                        m_stDetCfgData.bPolarity = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "enable_TP")
                    {
                        m_stDetCfgData.bEnableTP = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "equalise")
                    {
                        m_stDetCfgData.bEqualise = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "colourMode")
                    {
                        m_stDetCfgData.bColorMode = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "dataOutLines")
                    {
                        m_stDetCfgData.shDataOutLines =  atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "counterBitDepth")
                    {
                        m_stDetCfgData.shCounterBitDepth = atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "triggerMode")
                    {
                        m_stDetCfgData.shTriggerMode = atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "rowBlockSel")
                    {
                        m_stDetCfgData.shRowBlockSel = atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "counterMode")
                    {
                        m_stDetCfgData.nCounterMode = atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "chargeSum")
                    {
                        m_stDetCfgData.bChargeSum = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "gainMode")
                    {
                        m_stDetCfgData.shGainMode = atoi(vSplittedVal[1].c_str());
                    }
                    else if(strResult == "disc_SPM_CSM")
                    {
                        m_stDetCfgData.bDiscSPMCSM = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                    else if(strResult == "OMRheader")
                    {
                        m_stDetCfgData.bOMRHeader = (atoi(vSplittedVal[1].c_str())!=0);
                    }
                }
            }
        }
    }


    void LambdaConfigReader::ExtractDataFromChipConfig(vector<string> vStrChipCfg,int32 nIdx)
    {
        LOG_TRACE(__FUNCTION__);

        if(vStrChipCfg.size()==0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"No data can be extract for chip config!!!");
            return;
        }

        vector<string> vSplittedVal;
        string strDelimiter = " ";

        for(szt i=0;i<vStrChipCfg.size();i++)
        {
            if(!vSplittedVal.empty())
                vSplittedVal.clear();

            m_objStrUtil->StrSplit(vStrChipCfg[i],strDelimiter,vSplittedVal);

            for(szt j=0;j<vSplittedVal.size();j++)
            {
                string strResult = vSplittedVal[0];
                if(strResult == "senseDAC")
                {
                    m_vStCurrentChipData[nIdx].strSenseDAC = vSplittedVal[1];
                }
                else if(strResult == "extDAC")
                {
                    m_vStCurrentChipData[nIdx].strExtDAC = vSplittedVal[1];
                }
                else if(strResult == "preamp")
                {
                    m_vStCurrentChipData[nIdx].shPreamp = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "ikrum")
                {
                    m_vStCurrentChipData[nIdx].shIkrum = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "shaper")
                {
                    m_vStCurrentChipData[nIdx].shShaper= atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "disc")
                {
                    m_vStCurrentChipData[nIdx].shDisc = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "disc_LS")
                {
                    m_vStCurrentChipData[nIdx].shDiscLS = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "DAC_discL")
                {
                    m_vStCurrentChipData[nIdx].shDACDiscL = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "DAC_discH")
                {
                    m_vStCurrentChipData[nIdx].shDACDiscH = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "delay")
                {
                    m_vStCurrentChipData[nIdx].shDelay = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "TP_BufferIn")
                {
                    m_vStCurrentChipData[nIdx].shTPBufferIn = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "TP_BufferOut")
                {
                    m_vStCurrentChipData[nIdx].shTPBufferOut = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "RPZ")
                {
                    m_vStCurrentChipData[nIdx].shRPZ = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "GND")
                {
                    m_vStCurrentChipData[nIdx].shGND = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "TP_REF")
                {
                    m_vStCurrentChipData[nIdx].shTPREF = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "FBK")
                {
                    m_vStCurrentChipData[nIdx].shFBK = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "CAS")
                {
                    m_vStCurrentChipData[nIdx].shCAS = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "TP_REFA")
                {
                    m_vStCurrentChipData[nIdx].shTPREFA = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "TP_REFB")
                {
                    m_vStCurrentChipData[nIdx].shTPREFB = atoi(vSplittedVal[1].c_str());
                }
                else if(strResult == "keVtoThrSlopes")
                {
                    for(szt k=1;k<vSplittedVal.size();k++)
                        m_vStCurrentChipData[nIdx].vKeVToThrSlope[k-1]
                            = atof(vSplittedVal[k].c_str());
                    break;
                }
                else if(strResult == "thrBaselines")
                {
                    for(szt k=1;k<vSplittedVal.size();k++)
                        m_vStCurrentChipData[nIdx].vThrBaseline[k-1]
                            = atof(vSplittedVal[k].c_str());
                    break;
                }
            }
        }
    }
}
