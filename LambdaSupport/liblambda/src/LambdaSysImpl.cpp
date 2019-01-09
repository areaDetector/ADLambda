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

#include "LambdaGlobals.h"
#include "LambdaSysImpl.h"
#include "LambdaModule.h"
#include "ImageDecoder.h"
#include "LambdaTask.h"
#include "LambdaConfigReader.h"

namespace DetLambdaNS
{
    LambdaSysImpl::LambdaSysImpl()
    {
        LOG_TRACE(__FUNCTION__);

        //InitSys();    
    }

    LambdaSysImpl::LambdaSysImpl(string _strConfigPath)
        :m_strConfigFilePath(_strConfigPath)
    {
        LOG_TRACE(__FUNCTION__);
        m_emState = BUSY;
        SetState(BUSY);
        InitSys();
        SetState(ON);
    }

    LambdaSysImpl::~LambdaSysImpl()
    {
        LOG_TRACE(__FUNCTION__);
        ExitSys();
    }

    void LambdaSysImpl::InitSys()
    {
        LOG_TRACE(__FUNCTION__);

        float fDefaultEnergy = 6.0;
        m_strCurrentModuleName = "";
        m_strOperationMode= "";
        m_shTriggerMode = 0;
        m_dShutterTime = 2000;
        m_dDelayTime = 0;
        m_lFrameNo = 1;
        m_bSaveAllImg = false;
        m_bBurstMode = true;
        m_nThreshold = 0;
        m_fEnergy = fDefaultEnergy;
        //m_nState = 0;
        m_strSaveFilePath = "./";
        //m_strConfigFilePath = "";
        m_nX = -1;
        m_nY = -1;
        m_nSubimages = -1;
        m_nImgDepth = -1;

        m_bSlaveModule = false;
	
        m_lLastestImgNo = -1;
        m_lImgNo = -1;
        m_lQueueDepth = -1;
        m_lQueueImgNo = -1;
        m_vThreshold.resize(8,fDefaultEnergy);
        m_objMemPoolDecodedShort = NULL;
        m_objMemPoolDecodedInt = NULL;
        m_objMemPoolCompressed = NULL;
        
        m_objTask = NULL;
        m_nModuleType = 0;
        m_nTaskID = 0;
        m_bRunning = false;
        m_bSysExit = false;

        m_bCompressionEnabled = false;
        m_nCompressionLevel = 2;
        m_nCompressionMethod = 0;

        m_strModuleID = "unknown";
        m_strSystemType = "standard";
        
        m_nThreadNumbers = THREAD_NUMBER;
        m_nRawBufferLength = RAW_BUFFER_LENGTH;
        m_nDecodedBufferLength = DECODED_BUFFER_LENGTH;
        m_vNDecodingThreads.resize(3,2);

        // Decoding threads with HIGH priority
        m_vNDecodingThreads[0] = 3;

        // Threads with moderate NORMAL priority - run at lower frame rates
        m_vNDecodingThreads[1] = 3;
        
        // Threads with LOW priority that only run after all images received
        m_vNDecodingThreads[2] = 6;

        // Shutter time below which we should use fewer decoding threads while receiving
        m_dCriticalShutterTime = 2.1;

        m_nDistortionCorrectionMethod = 1;

        m_objConfigReader = new LambdaConfigReader(m_strConfigFilePath);
        
        ReadConfig(false,"");
        InitNetwork();
        InitMemoryPool();
        InitThreadPool();
        
        //cout<<m_nThreadNumbers<<"|"<< m_nRawBufferLength<<"|"<<m_nDecodedBufferLength<<endl;

        SetOperationMode(m_strOperationMode);

        m_bAcquisitionStart = false;
        m_bAcquisitionStop = true;
        m_ptrnLiveData = new int32[m_nDistortedImageSize];
        std::fill(m_ptrnLiveData,m_ptrnLiveData+m_nDistortedImageSize,0);
        m_hasFirstImageArrived = false;
        m_lLiveFrameNo = -1;
        m_shLiveErrCode = -1;

        SetThreshold(m_nThreshold, m_fEnergy);
        SetShutterTime(m_dShutterTime);
        SetTriggerMode(m_shTriggerMode);
        SetNImages(m_lFrameNo);
        SetBurstMode(m_bBurstMode);
        
        JoinAllDecodeTasks();
        
        ///quick fix for reading out unsual data
        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];
        szt nPacketSize = UDP_PACKET_SIZE_NORMAL;
        int32 nCount = 50;
        int16 shErrorCode = 0;
        int32 nNoDataTimes = 0;

        usleep(500000); // Wait to ensure bad packets are caught
                    
        for(szt i=1;i<m_vNetInterface.size();i++)
        {
            nNoDataTimes = 0;
            while(true)
            {
                shErrorCode = m_vNetInterface[i]->ReceivePacket(ptrchPacket,nPacketSize);
                if(shErrorCode==-1)
                {
                    nNoDataTimes++;
                    //cout << "Timeouttest ";
                    if(nNoDataTimes==nCount)
                        break;
                }
                else 
                {
                    nNoDataTimes = 0;
                    //cout << "Extrapacket ";
                }
            }
        }
        delete[] ptrchPacket;
        ptrchPacket = NULL;

        // Throw in a test of Read OMR
        for(szt i=0; i<m_vCurrentChip.size();i++)
        {
            int32 currentChip = m_vCurrentChip[i];
	    if((m_strSystemType == "hexa") && (currentChip >=4) && (currentChip <=9)) continue; // Hack for hexa (discrepancy between chips present and image size - want to skip nonexistent chips	
            string IDtest = m_objLambdaModule->GetChipID(currentChip);
            if(i == 0)
                m_strModuleID = IDtest;
            // std::cout << "ID of chip " << currentChip << " is " << IDtest << "\n";
        }
                
        m_bRunning = true;
    }

    void LambdaSysImpl::ExitSys()
    {
        LOG_TRACE(__FUNCTION__);
            
        for(szt i=0;i<m_vNetInterface.size();i++)
            m_vNetInterface[i]->Disconnect();

        m_bSysExit = true;

        delete m_objMemPoolRaw;
        m_objMemPoolRaw = NULL;

        delete m_objMemPoolDecodedShort;
        m_objMemPoolDecodedShort = NULL;

        delete m_objMemPoolDecodedInt;
        m_objMemPoolDecodedInt = NULL;

        delete m_objMemPoolCompressed;
        m_objMemPoolCompressed = NULL;

        delete m_objThPool;
        m_objThPool = NULL;
        
        m_ptrshDecodeImg = NULL;
    
        m_ptrnDecodeImg = NULL;

        delete m_objConfigReader;
        m_objConfigReader = NULL;

        m_objFileOp = NULL;

        delete m_objLambdaModule;
        m_objLambdaModule = NULL;

        delete[] m_ptrnLiveData;
        m_ptrnLiveData = NULL;
       
    }
    
    void LambdaSysImpl::ReadConfig(bool bSwitchMode, string strOpMode)
    {
        LOG_TRACE(__FUNCTION__);
       
        //destination port No for CH1
        m_objConfigReader->LoadLocalConfig(bSwitchMode, strOpMode);
        m_bMultilink = m_objConfigReader->GetMultilink();
        m_bBurstMode = m_objConfigReader->GetBurstMode();
        m_strSystemType = m_objConfigReader->GetSystemType();
        m_objConfigReader->GetUDPConfig(m_vStrMAC,m_vStrIP,m_vUShPort);
        m_objConfigReader->GetTCPConfig(m_strTCPIPAddress,m_shTCPPortNo);
        m_objConfigReader->GetChipConfig(m_vCurrentChip,m_vStCurrentChipData);
        m_strOperationMode = m_objConfigReader->GetOperationMode();
        m_strCurrentModuleName = m_objConfigReader->GetModuleName();
        m_stDetCfgData = m_objConfigReader->GetDetConfigData();

        m_nRawBufferLength = m_objConfigReader->GetRawBufferLength();
        m_nDecodedBufferLength = m_objConfigReader->GetDecodedBufferLength();
        m_vNDecodingThreads = m_objConfigReader->GetDecodingThreadNumbers();
        m_dCriticalShutterTime = m_objConfigReader->GetCriticalShutterTime();

        m_bSlaveModule = m_objConfigReader->GetSlaveModule();
        
        m_vfTranslation = m_objConfigReader->GetPosition();


        m_emReadoutMode = GetReadoutModeEnum(m_strOperationMode);

        if(m_nDistortionCorrectionMethod == 1)
        {
            
            m_objConfigReader->GetDistoredImageSize(m_nDistortedX,m_nDistortedY);
            if((m_nDistortedX < 0) || (m_nDistortedY < 0))
            {
                m_nDistortionCorrectionMethod = 0; // Fall back to no distortion correction
            }
            else
            {

                m_nX = m_nDistortedX;
                m_nY = m_nDistortedY;

                m_vUnPixelMask = m_objConfigReader->GetPixelMask();
                m_vNIndex = m_objConfigReader->GetIndexFile();
                m_vNNominator = m_objConfigReader->GetNominatorFile();
            }
        }
	
        if(m_nDistortionCorrectionMethod == 0)
        {
            // Temporarily create image decoder, to conveniently find decoded image size
            ImageDecoder* objTempImageDecoder = new ImageDecoder(m_vCurrentChip);
            objTempImageDecoder->GetDecodedImageSize(m_nX, m_nY);
            delete objTempImageDecoder;
        }
        else if(m_nDistortionCorrectionMethod == 1)
        {
            
            m_objConfigReader->GetDistoredImageSize(m_nDistortedX,m_nDistortedY);
            if((m_nDistortedX < 0) || (m_nDistortedY < 0))
            {
                m_nDistortionCorrectionMethod = 0; // Fall back to no distortion correction
            }
            else
            {

                m_nX = m_nDistortedX;
                m_nY = m_nDistortedY;
                m_vUnPixelMask = m_objConfigReader->GetPixelMask();
                m_vNIndex = m_objConfigReader->GetIndexFile();
                m_vNNominator = m_objConfigReader->GetNominatorFile();
            }
        }
	
        if(m_nDistortionCorrectionMethod == 0)
        {
            // Temporarily create image decoder, to conveniently find decoded image size
            ImageDecoder* objTempImageDecoder = new ImageDecoder(m_vCurrentChip);
            objTempImageDecoder->GetDecodedImageSize(m_nX, m_nY);
            delete objTempImageDecoder;
        }



        m_nDistortedImageSize = m_nX*m_nY;

        if(m_emReadoutMode == OPERATION_MODE_24)
            m_nImgDepth = 24;
        else 
            m_nImgDepth = 12;


        m_nSubimages = 1;
        
        // Have 2 thresholds in 2x12 mode currently
        if(m_emReadoutMode == OPERATION_MODE_2x12) m_nSubimages = 2; 

        //depends on the the number of chips. Decide the image size
        if(m_vCurrentChip.size() == 12)
        {
            // m_nX = 1536;
            // m_nY = 512;
            m_nModuleType = 1;
            m_nImgDataSize = IMAGE_LENGTH_12_CHIP_IN_BYTE;
            m_nPacketsNumber = PACKET_NUMBERS_12_CHIP;
        }
        else if(m_vCurrentChip.size() == 6)
        {
            // m_nX = 768;
            // m_nY = 512;
            m_nModuleType = 2;
            m_nImgDataSize = IMAGE_LENGTH_6_CHIP_IN_BYTE;
            m_nPacketsNumber = PACKET_NUMBERS_6_CHIP;
        }

        // Calculate the image size and no of packets based on the no of chips
        // Note - for 1-chip tests, might need to "force" the image size to
        // simplify firmware tests (e.g. test 1 chip without having to change data output)
        m_nImgDataSize = m_vCurrentChip.size()  * (BYTES_IN_CHIP  + CHIP_HEADER_SIZE);
        m_nPacketsNumber = m_nImgDataSize / (UDP_PACKET_SIZE_NORMAL - UDP_EXTRA_BYTES);

        // No of packets is total data divided by data per packet, rounded up
        if((m_nImgDataSize / (UDP_PACKET_SIZE_NORMAL - UDP_EXTRA_BYTES)) != 0) m_nPacketsNumber++;
    }
    
    bool LambdaSysImpl::InitNetwork()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_bMultilink)
        {
            //establish TCP link first
            NetworkImplementation* objNetImplTCP
                = new NetworkTCPImplementation(m_strTCPIPAddress,m_shTCPPortNo);
            NetworkInterface* objNetInterfaceTCP
                = new NetworkTCPInterface(objNetImplTCP);
            m_vNetInterface.push_back(objNetInterfaceTCP);
            
            ///once TCP is connected successfully, config UDP connections
            if(m_vNetInterface[0]->Connect()==0)
            {
                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,
                                                     m_vNetInterface[0],
                                                     m_bMultilink,
                                                     m_vCurrentChip,
                                                     m_stDetCfgData,
                                                     m_vStCurrentChipData,
                                                     m_bSlaveModule,
                                                     m_strSystemType);

                //send udp data via TCP
                m_objLambdaModule->WriteUDPMACAddress(0,m_vStrMAC[0]);
                m_objLambdaModule->WriteUDPMACAddress(1,m_vStrMAC[1]);

                m_objLambdaModule->WriteUDPIP(0,m_vStrIP[0]);
                m_objLambdaModule->WriteUDPIP(1,m_vStrIP[1]);
            
                m_objLambdaModule->WriteUDPPorts(0,m_vUShPort);
                m_objLambdaModule->WriteUDPPorts(1,m_vUShPort);
                
                NetworkImplementation* objNetImplUDP
                    = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[1]);
                NetworkImplementation* objNetImplUDP1
                    = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[2]);
                NetworkImplementation* objNetImplUDP2
                    = new NetworkUDPImplementation(m_vStrIP[1][0],m_vUShPort[1]);
                NetworkImplementation* objNetImplUDP3
                    = new NetworkUDPImplementation(m_vStrIP[1][0],m_vUShPort[2]);
            
                NetworkInterface* objNetInterfaceUDP = new NetworkUDPInterface(objNetImplUDP);
                NetworkInterface* objNetInterfaceUDP1 = new NetworkUDPInterface(objNetImplUDP1);
                NetworkInterface* objNetInterfaceUDP2 = new NetworkUDPInterface(objNetImplUDP2);
                NetworkInterface* objNetInterfaceUDP3 = new NetworkUDPInterface(objNetImplUDP3);

                m_vNetInterface.push_back(objNetInterfaceUDP);
                m_vNetInterface.push_back(objNetInterfaceUDP1);
                m_vNetInterface.push_back(objNetInterfaceUDP2);
                m_vNetInterface.push_back(objNetInterfaceUDP3);
                
                for(szt i=1;i<m_vNetInterface.size();i++)
                    m_vNetInterface[i]->Connect();
                return true;
            }
            else
            {
                std::cout << "Unable to connect to detector. Please check that detector is powered and connected." << "\n";
                return false;
            }
        }
        else
        {
            //init network
            NetworkImplementation* objNetImplTCP
                = new NetworkTCPImplementation(m_strTCPIPAddress,m_shTCPPortNo);         
            NetworkInterface* objNetInterfaceTCP
                = new NetworkTCPInterface(objNetImplTCP);
            
            m_vNetInterface.push_back(objNetInterfaceTCP);
            
            if(m_vNetInterface[0]->Connect() == 0)
            {

                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,
                                                     m_vNetInterface[0],
                                                     m_bMultilink,
                                                     m_vCurrentChip,
                                                     m_stDetCfgData,
                                                     m_vStCurrentChipData,
                                                     m_bSlaveModule,
                                                     m_strSystemType);

                //send udp data via TCP
                m_objLambdaModule->WriteUDPMACAddress(0,m_vStrMAC[0]);
                m_objLambdaModule->WriteUDPIP(0,m_vStrIP[0]);
                m_objLambdaModule->WriteUDPPorts(0,m_vUShPort);

                if(m_bBurstMode)
                {
                    NetworkImplementation* objNetImplData
                        = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[1]);
                    NetworkInterface* objNetInterfaceData
                        = new NetworkUDPInterface(objNetImplData);
                    m_vNetInterface.push_back(objNetInterfaceData);
                    m_vNetInterface[1]->Connect();
                }
                else
                {
                    //Use TCP interface - for now, have "magic number" 3490
                    //as port but should implement more flexibly
                    NetworkImplementation* objNetImplData
                        = new NetworkTCPImplementation(m_strTCPIPAddress,3490);
                    NetworkInterface* objNetInterfaceData
                        = new NetworkTCPInterface(objNetImplData);
                    m_vNetInterface.push_back(objNetInterfaceData);
                    m_vNetInterface[1]->Connect();		    
                }

                return true;
            }
            else
            {
                std::cout << "Unable to connect to detector. Please check that detector is powered and connected." << "\n";
                return false;
            }
        }
    }

    void LambdaSysImpl::InitThreadPool()
    {
        LOG_TRACE(__FUNCTION__);
            
        //init thread pool
        m_objThPool = new ThreadPool(m_nThreadNumbers);	
        usleep(100);
    }

    void LambdaSysImpl::InitMemoryPool()
    {
        LOG_TRACE(__FUNCTION__);
        if((m_emReadoutMode == OPERATION_MODE_12) || (m_emReadoutMode == OPERATION_MODE_2x12))
            m_objMemPoolDecodedShort
                = new MemPool<int16>(m_nDecodedBufferLength,m_nDistortedImageSize);
        else if(m_emReadoutMode == OPERATION_MODE_24)
            m_objMemPoolDecodedInt
                = new MemPool<int32>(m_nDecodedBufferLength,m_nDistortedImageSize);
        usleep(100);
        m_objMemPoolRaw = new MemPool<char>(m_nRawBufferLength,m_nImgDataSize);
        usleep(100);
    }

    string LambdaSysImpl::GetSensorMaterial()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_strCurrentModuleName.find("FullRX")!=std::string::npos)
            return string("Si");
        else if(m_strCurrentModuleName.find("GaAs")!=std::string::npos)
            return string("GaAs");
        else if(m_strCurrentModuleName.find("CdTe")!=std::string::npos)
            return string("CdTe");
        else
            return string("UNKNOWN");
        
    }

    string LambdaSysImpl::GetModuleID()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strModuleID;
    }

    string LambdaSysImpl::GetSystemInfo()
    {
        return string("liblambda version:")
            + LAMBDA_VERSION
            + string(" -- firmware version:")
            + m_objLambdaModule->GetFirmwareVersion();
    }

    string LambdaSysImpl::GetDetCoreVersion()
    {
        return FSDetCoreNS::FSDETCORE_VERSION;
    }

    string LambdaSysImpl::GetLibLambdaVersion()
    {
        return LAMBDA_VERSION;
    }

    string LambdaSysImpl::GetFirmwareVersion()
    {
        return m_objLambdaModule->GetFirmwareVersion();
    }
    
    string LambdaSysImpl::GetCalibFile()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strConfigFilePath+string("/")+m_strCurrentModuleName+string("/lookups/")
            +m_strCurrentModuleName+string("_calib.nxs");
    }
    vector<float> LambdaSysImpl::GetPosition()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vfTranslation;
    }

    Enum_readout_mode LambdaSysImpl::GetReadoutModeEnum(string strOperationMode)
    {
        Enum_readout_mode emNewMode;
        LOG_TRACE(__FUNCTION__);
        // For time being, hard-code options -
        // in future, should make this more flexible (or carefully tailored)
        if(strOperationMode=="ContinuousReadWrite") emNewMode = OPERATION_MODE_12;
        else if(strOperationMode=="TwentyFourBit") emNewMode = OPERATION_MODE_24;
        else if(strOperationMode=="DualThreshold") emNewMode = OPERATION_MODE_2x12;
        else if(strOperationMode=="ContinuousReadWriteCSM") emNewMode = OPERATION_MODE_12;
        else if(strOperationMode=="TwentyFourBitCSM") emNewMode = OPERATION_MODE_24;
        else if(strOperationMode=="DualThresholdCSM") emNewMode = OPERATION_MODE_2x12;
        else if(strOperationMode=="HighSpeedBurst") emNewMode = OPERATION_MODE_12;

        else emNewMode = OPERATION_MODE_UNKNOWN;
        return emNewMode;
    }  
    
        
    void LambdaSysImpl::SetOperationMode(string strOperationMode)
    {
        LOG_TRACE(__FUNCTION__);
        // In initial implementation, hard-code these

        m_emReadoutMode = GetReadoutModeEnum(strOperationMode);

        SetState(BUSY);
        if(m_emReadoutMode != OPERATION_MODE_UNKNOWN)
        {
            if(m_strOperationMode != strOperationMode && m_bRunning)
            {
                StopAllDecodeTasks(); // Stop decoding tasks before making changes 
                //reload configuration files
                m_strOperationMode = strOperationMode;
                ReadConfig(true,m_strOperationMode);
                delete m_objLambdaModule;
                
                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,
                                                     m_vNetInterface[0],
                                                     m_bMultilink,
                                                     m_vCurrentChip,
                                                     m_stDetCfgData,
                                                     m_vStCurrentChipData,
                                                     m_bSlaveModule,
                                                     m_strSystemType);
		
                //cout << "Existing threshold = " << m_nThreshold
                //<< " and energy = " << m_fEnergy << endl;            
                if((m_emReadoutMode == OPERATION_MODE_12)
                   || (m_emReadoutMode == OPERATION_MODE_2x12))
                {
                    delete m_objMemPoolDecodedInt;
                    m_objMemPoolDecodedInt = NULL;
                    m_objMemPoolDecodedShort
                        = new MemPool<int16>(m_nDecodedBufferLength,m_nDistortedImageSize);
                    usleep(100);
                }               
                else if(m_emReadoutMode == OPERATION_MODE_24)
                {
                    delete m_objMemPoolDecodedShort;
                    m_objMemPoolDecodedShort = NULL;
                    m_objMemPoolDecodedInt
                        = new MemPool<int32>(m_nDecodedBufferLength,m_nDistortedImageSize);
                    usleep(100);
                }
               
                
                SetThreshold(m_nThreshold, m_fEnergy); 
                SetShutterTime(m_dShutterTime);
                SetTriggerMode(m_shTriggerMode);
                SetNImages(m_lFrameNo);
                SetBurstMode(m_bBurstMode);

                JoinAllDecodeTasks(); // Start the decode tasks again
                
            }
        }
        else
            LOG_STREAM(__FUNCTION__,ERROR,
                       ("This Operation Mode( "+strOperationMode +") does not exist!"));
        
        SetState(ON);
    }

    string LambdaSysImpl::GetOperationMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strOperationMode;           
    }

    void LambdaSysImpl::SetTriggerMode(int16 shTriggerMode)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_shTriggerMode = shTriggerMode;
        m_objLambdaModule->WriteTriggerMode(shTriggerMode);
        SetState(ON);
    }

    int16 LambdaSysImpl::GetTriggerMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_shTriggerMode;            
    }

    void LambdaSysImpl::SetShutterTime(double dShutterTime)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_dShutterTime = dShutterTime;
        m_objLambdaModule->WriteShutterTime(dShutterTime);
        SetState(ON);
    }

    double LambdaSysImpl::GetShutterTime()
    {
        LOG_TRACE(__FUNCTION__);
        return m_dShutterTime;
    }

    void LambdaSysImpl::SetDelayTime(double dDelayTime)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_dDelayTime = dDelayTime;
        m_objLambdaModule->WriteDelayTime(dDelayTime);
        SetState(ON);
    }

    double LambdaSysImpl::GetDelayTime()
    {
        LOG_TRACE(__FUNCTION__);
        return m_dDelayTime;
    }

    void LambdaSysImpl::SetNImages(int32 lImages)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_lFrameNo = lImages;
        m_objLambdaModule->WriteImageNumbers(lImages);                
        SetState(ON);
    }

    int32 LambdaSysImpl::GetNImages()
    {
        LOG_TRACE(__FUNCTION__);
        return m_lFrameNo; 
    }

    void LambdaSysImpl::SetSaveAllImages(bool bSaveAllImgs)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_bSaveAllImg = bSaveAllImgs;
        SetState(ON);
    }

    bool LambdaSysImpl::GetSaveAllImages()
    {
        LOG_TRACE(__FUNCTION__);
        return m_bSaveAllImg;
    }  

    void LambdaSysImpl::SetBurstMode(bool bBurstMode)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_bBurstMode = bBurstMode;
        int nVal = (bBurstMode==true?1:0);
        m_objLambdaModule->WriteNetworkMode(nVal);
        SetState(ON);
    }
        
    bool LambdaSysImpl::GetBurstMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_bBurstMode;
    }

    void LambdaSysImpl::SetThreshold(int32 nThresholdNo, float fEnergy)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_nThreshold = nThresholdNo;
        m_vThreshold[nThresholdNo] = fEnergy;
        m_fEnergy = fEnergy;
        m_objLambdaModule->WriteEnergyThreshold(nThresholdNo,fEnergy);
        SetState(ON);
    }

    float LambdaSysImpl::GetThreshold(int32 nThresholdNo)
    {
        LOG_TRACE(__FUNCTION__);
        m_nThreshold = nThresholdNo;
        m_fEnergy = m_vThreshold[nThresholdNo];
        return m_vThreshold[nThresholdNo];
    }
  
    void LambdaSysImpl::SetState(Enum_detector_state emState)
    {
        LOG_TRACE(__FUNCTION__);
        // When exiting RECEIVING_IMAGES state,
        // we want to make sure detector module is ready for next acquisition
        if((m_emState == RECEIVING_IMAGES) && (emState != RECEIVING_IMAGES))
        {
            m_objLambdaModule->PrepNextImaging();
        }
        m_emState = emState;
    }

    Enum_detector_state LambdaSysImpl::GetState()
    {
        //LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstMtxSync); 
        // If imaging is running, need to check on progress here and update if necessary
        // States 4 and above are RECEIVING_IMAGES, PROCESSING_IMAGES, FINISHED
        if (m_emState >= 4) 
        {
            //Check for raw images complete  
            int expectedframes = m_lFrameNo;
            int expectedRawFrames = m_lFrameNo;

            if((m_emReadoutMode == OPERATION_MODE_24) || (m_emReadoutMode == OPERATION_MODE_2x12))
                expectedRawFrames*=2;
                
            
            int rawframes = m_objMemPoolRaw->GetTotalReceivedFrames();

            if(rawframes<expectedRawFrames)
                SetState(RECEIVING_IMAGES);
            else
            {
                int decodedframes = 0;
                if(!m_bCompressionEnabled)
                {
                    if((m_emReadoutMode == OPERATION_MODE_12)
                       || (m_emReadoutMode == OPERATION_MODE_2x12))
                        decodedframes =  m_objMemPoolDecodedShort->GetTotalReceivedFrames();
                    else if(m_emReadoutMode == OPERATION_MODE_24)
                        decodedframes =  m_objMemPoolDecodedInt->GetTotalReceivedFrames();
                }
                else
                {
                    decodedframes = m_objMemPoolCompressed->GetTotalReceivedFrames();
                }
                
                if ((decodedframes < expectedframes)||(GetQueueDepth()>0))
                {
                    if (m_emState != 5) 
                    {
                        SetState(PROCESSING_IMAGES);
                        // Allow low priority tasks to run if image collection is
                        // done but we have images to process
                        m_objThPool->SetPriorityLevel(LOW); 
                    }
                }
                else
                    // This indicates library done, BUT that we haven't reset state
                    // to ON with StopFastImaging.
                    // The FINISHED state is to help Tango -
                    // it allows the library to remain in a "not ON" state until
                    // the Tango server is finished with files etc.
                    SetState(FINISHED); 
            }
        }

        //cout << "Current state is " << m_emState << "\n";
        return m_emState;
    }


    void LambdaSysImpl::SetSaveFilePath(string strSaveFilePath)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_strSaveFilePath = strSaveFilePath;
        SetState(ON);
    }

    string LambdaSysImpl::GetSaveFilePath()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strSaveFilePath;
            
    }

    void LambdaSysImpl::SetConfigFilePath(string strConfigFilePath)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_strConfigFilePath = strConfigFilePath;  
        SetState(ON);          
    }

    string LambdaSysImpl::GetConfigFilePath()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strConfigFilePath;
    }
    
    void LambdaSysImpl::StartImaging()
    {
        LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstMtxSync);
        SetState(RECEIVING_IMAGES);
        if(m_dShutterTime > m_dCriticalShutterTime)
        {
            // Go to normal priority for task processing
            m_objThPool->SetPriorityLevel(NORMAL); 
        }
        else
        {
            // Limit number of decoders to improve reliability at high frame rates
            m_objThPool->SetPriorityLevel(HIGH); 
        }
        std::fill(m_ptrnLiveData,m_ptrnLiveData+m_nDistortedImageSize,0);
	m_hasFirstImageArrived = false;
        //cout << "Putting into receiving state \n";
        m_nTaskID = 0;
        //reset once, for avoiding the images come after stopacq command 
        m_objMemPoolRaw->Reset();
        m_objMemPoolRaw->SetRequestedPacketNumber(m_nPacketsNumber);


        if(!m_bCompressionEnabled)
        {
            if((m_emReadoutMode==OPERATION_MODE_12) || (m_emReadoutMode==OPERATION_MODE_2x12))
                m_objMemPoolDecodedShort->Reset();
            else if (m_emReadoutMode== OPERATION_MODE_24)
                m_objMemPoolDecodedInt->Reset();
        }
        else
            m_objMemPoolCompressed->Reset();
      

        m_bAcquisitionStart = true;
        m_bAcquisitionStop = false;
        lock.unlock();

        if(!m_bMultilink)
        {
            if(m_bBurstMode)
                CreateTask(m_strOperationMode,"StartAcquisition");
            else
                CreateTask(m_strOperationMode,"StartAcquisitionTCP");
        }
        else
        {   
            for(szt i=1;i<m_vNetInterface.size();i++)
                CreateTask(m_strOperationMode,"MultiLink",m_vNetInterface[i]);
            
            CreateTask(m_strOperationMode,"MonitorTask"); 
        }

        m_objLambdaModule->StartFastImaging();
    }

    void LambdaSysImpl::StopImaging()
    {
        LOG_TRACE(__FUNCTION__);

        // Only use LambdaModule command to stop imaging
        // if it's currently receiving images - otherwise nothing needs to be done
        if(GetState()==RECEIVING_IMAGES)
            m_objLambdaModule->StopFastImaging();

        boost::unique_lock<boost::mutex> lock(m_bstMtxSync);
        // Do all of this during locked mutex -
        // don't allow state updates during this process for safety!
        m_bAcquisitionStart = false;
        m_bAcquisitionStop = true;
    
        //here reset once, in order to let the decoding thread finishing the rest decoding work
        m_objMemPoolRaw->Reset();
        if(!m_bCompressionEnabled)
        {
            
            if(m_emReadoutMode == OPERATION_MODE_12)
                m_objMemPoolDecodedShort->Reset();
            else if (m_emReadoutMode == OPERATION_MODE_24)
                m_objMemPoolDecodedInt->Reset();
        }
        else
            m_objMemPoolCompressed->Reset();
        
        usleep(100);
        SetState(ON);
    }

    void LambdaSysImpl::SetCompressionEnabled(bool bCompressionEnabled,int32 nCompLevel)
    {
        LOG_TRACE(__FUNCTION__);

        StopAllDecodeTasks();
	
        m_bCompressionEnabled = bCompressionEnabled;
        m_nCompressionLevel = nCompLevel;

        if(m_bCompressionEnabled)
        {
            if(m_objMemPoolCompressed == NULL)
            {
                // Allocate 5 bytes of memory for each pixel in compressed image to avoid overflow
                int safetyFactor = 5; 

                m_objMemPoolCompressed = new MemPool<char>(
                    m_nDecodedBufferLength,m_nDistortedImageSize*safetyFactor);
                usleep(100);
            }
        }

        JoinAllDecodeTasks();
    }

    void LambdaSysImpl::GetCompressionEnabled(bool& bCompressionEnabled,int32& nCompLevel)
    {
        LOG_TRACE(__FUNCTION__);

        bCompressionEnabled = m_bCompressionEnabled;
        nCompLevel = m_nCompressionLevel;
    }
    
    
    int LambdaSysImpl::GetCompressionMethod()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nCompressionMethod;
    }
    

    vector<uint32> LambdaSysImpl::GetPixelMask()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vUnPixelMask;
    }

    void LambdaSysImpl::SetDistortionCorrecttionMethod(int32 nMethod)
    {
        LOG_TRACE(__FUNCTION__);

        //new mode
        if(nMethod!=m_nDistortionCorrectionMethod && nMethod<=1 && nMethod>=0 )
        {
            m_nDistortionCorrectionMethod = nMethod;
            ReadConfig(true,m_strOperationMode);
        }
    }

    int32 LambdaSysImpl::GetDistortionCorrecttionMethod()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nDistortionCorrectionMethod;
    }
        
    void LambdaSysImpl::GetImageFormat(int32& nX, int32& nY, int32& nImgDepth)
    {
        LOG_TRACE(__FUNCTION__);
        nX = m_nX;
        nY = m_nY;
        nImgDepth = m_nImgDepth;
    }

    int32 LambdaSysImpl::GetNSubimages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nSubimages;
    }

    // Helper function - returns enum corresponding to Medipix3 chip readout mode
    Enum_readout_mode LambdaSysImpl::GetReadoutModeCode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_emReadoutMode;
    }
  
    int32 LambdaSysImpl::GetLatestImageNo()
    {
        LOG_TRACE(__FUNCTION__);
        return m_lLastestImgNo;
    }

    int32 LambdaSysImpl::GetQueueDepth()
    {
        //LOG_TRACE(__FUNCTION__);

        if(!m_bCompressionEnabled)
        {
            
            if(m_emReadoutMode == OPERATION_MODE_24)
                m_lQueueDepth =  m_objMemPoolDecodedInt->GetStoredImageNumbers();
            else if((m_emReadoutMode == OPERATION_MODE_12)
                    || (m_emReadoutMode == OPERATION_MODE_2x12))
                m_lQueueDepth =  m_objMemPoolDecodedShort->GetStoredImageNumbers();
        }
        else
            m_lQueueDepth =  m_objMemPoolCompressed->GetStoredImageNumbers();
        
        
        if(m_lQueueDepth>0)
            LOG_INFOS(("Currently stored images are:"+to_string(m_lQueueDepth)));
        return m_lQueueDepth;
    }

    int32 LambdaSysImpl::GetFreeBufferSize()
    {
        return (m_nRawBufferLength - (m_objMemPoolRaw->GetStoredImageNumbers()));
    }

    void LambdaSysImpl::GetRawImage(char* ptrchRetImg,int32& lFrameNo,int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
        //m_objMemPoolRaw->GetImage(ptrchRetImg,lFrameNo,shErrCode);
    }

    int32* LambdaSysImpl::GetDecodedImageInt(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
        int32 nDataLength;
      
        if(m_objMemPoolDecodedInt->IsImageReadyForReading(1))
        {
            m_objMemPoolDecodedInt->GetImage(m_ptrnDecodeImg,lFrameNo,shErrCode,nDataLength);  
            return m_ptrnDecodeImg;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return NULL;
        }
    }
  
    int16* LambdaSysImpl::GetDecodedImageShort(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
        int32 nDataLength;
        
        if(m_objMemPoolDecodedShort->IsImageReadyForReading(1))
        {
            m_objMemPoolDecodedShort->GetImage(m_ptrshDecodeImg,lFrameNo,shErrCode,nDataLength);
            return m_ptrshDecodeImg;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return NULL;
        }

    }
    
    char* LambdaSysImpl::GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength)
    {
        LOG_TRACE(__FUNCTION__);

        if(m_objMemPoolCompressed->IsImageReadyForReading(1))
        { 
            m_objMemPoolCompressed->GetImage(m_ptrchData,lFrameNo,shErrCode,nDataLength); 
            return m_ptrchData;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return NULL;
        }
        
    }

    void LambdaSysImpl::SetCurrentImage(int16* ptrshImg,int32 lFrameNo,int16 shErrCode)
    {
        uint32 nFPS = static_cast<uint32>(1*1000/((m_dShutterTime)*10));
        if(nFPS == 0)
            nFPS = 1;
        if((lFrameNo % nFPS == 0) || !m_hasFirstImageArrived)
        {
            m_lLiveFrameNo = lFrameNo;
            m_shLiveErrCode = shErrCode;
            std::copy(ptrshImg,ptrshImg+m_nDistortedImageSize,m_ptrnLiveData);
	    m_hasFirstImageArrived = true;
        }         
    }

    void LambdaSysImpl::SetCurrentImage(int32* ptrnImg,int32 lFrameNo,int16 shErrCode)
    {
        uint32 nFPS = static_cast<uint32>(1*1000/((m_dShutterTime)*10));
        if(nFPS == 0)
            nFPS = 1;
        if((lFrameNo % nFPS == 0) || !m_hasFirstImageArrived)
        {
            m_lLiveFrameNo = lFrameNo;
            m_shLiveErrCode = shErrCode;
            std::copy(ptrnImg,ptrnImg+m_nDistortedImageSize,m_ptrnLiveData);
	    m_hasFirstImageArrived = true;
        }
         
    }
    
    int32* LambdaSysImpl::GetCurrentImage(int32& lFrameNo,int16& shErrCode)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtxSync);
        lFrameNo = m_lLiveFrameNo;
        shErrCode = m_shLiveErrCode;
        return m_ptrnLiveData;
    }
    
    
    bool LambdaSysImpl::GetAcquisitionStart() const
    {
        return m_bAcquisitionStart;
    }

    bool LambdaSysImpl::GetAcquisitionStop() const
    {
        return m_bAcquisitionStop;
    }

    void LambdaSysImpl::SetAcquisitionStart(const bool bStatus)
    {
        m_bAcquisitionStart = bStatus;
    }

    void LambdaSysImpl::SetAcquisitionStop(const bool bStatus)
    {
        m_bAcquisitionStop = bStatus;
    }

    bool LambdaSysImpl::SysExit() const
    {
        return m_bSysExit;
    }


    void LambdaSysImpl::CreateTask(string strOpMode,string strTaskName)
    {
        LOG_TRACE(__FUNCTION__);

        Task* objTask = NULL;

        if(!m_bCompressionEnabled)
        {
            
            if((m_emReadoutMode == OPERATION_MODE_12) || (m_emReadoutMode == OPERATION_MODE_2x12))
                objTask = new LambdaTask(strTaskName,HIGH,1,this,m_vNetInterface[1],
                                         m_objMemPoolRaw,m_objMemPoolDecodedShort,
                                         &m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
            
            else if(m_emReadoutMode == OPERATION_MODE_24)
                objTask = new LambdaTask(strTaskName,HIGH,1,this,m_vNetInterface[1],
                                         m_objMemPoolRaw,m_objMemPoolDecodedInt,
                                         &m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        }
        else
            objTask = new LambdaTask(strTaskName,HIGH,1,this,m_vNetInterface[1],
                                     m_objMemPoolRaw,m_objMemPoolCompressed,
                                     &m_bstMtxSync,m_vCurrentChip,m_vNIndex,
                                     m_vNNominator,m_nDistortedImageSize);
        
        
        m_objThPool->AddTask(objTask);
    }

    void LambdaSysImpl::CreateTask(string strOpMode,
                                   string strTaskName,
                                   Enum_priority enumTaskPriority)
    {
        LOG_TRACE(__FUNCTION__);

        Task* objTask = NULL;
        if(!m_bCompressionEnabled)
        {
            
            if((m_emReadoutMode == OPERATION_MODE_12) || (m_emReadoutMode == OPERATION_MODE_2x12))
                objTask = new LambdaTask(strTaskName,enumTaskPriority,1,this,
                                         m_vNetInterface[1],m_objMemPoolRaw,
                                         m_objMemPoolDecodedShort,&m_bstMtxSync,
                                         m_vCurrentChip,m_vNIndex,m_vNNominator);
            else if(m_emReadoutMode == OPERATION_MODE_24) 
                objTask = new LambdaTask(strTaskName,enumTaskPriority,1,this,
                                         m_vNetInterface[1],m_objMemPoolRaw,
                                         m_objMemPoolDecodedInt,&m_bstMtxSync,
                                         m_vCurrentChip,m_vNIndex,m_vNNominator);
        }
        else
            objTask = new LambdaTask(strTaskName,enumTaskPriority,1,this,
                                     m_vNetInterface[1],m_objMemPoolRaw,
                                     m_objMemPoolCompressed,&m_bstMtxSync,
                                     m_vCurrentChip,m_vNIndex,m_vNNominator,
                                     m_nDistortedImageSize);

        m_objThPool->AddTask(objTask);
    }


    void LambdaSysImpl::CreateTask(string strOpMode,
                                   string strTaskName,
                                   NetworkInterface* objNetInterface)
    {      
        Task*  objTask = NULL;
        if(!m_bCompressionEnabled)
        {
            
            if((m_emReadoutMode == OPERATION_MODE_12) || (m_emReadoutMode == OPERATION_MODE_2x12))
                objTask = new LambdaTask(strTaskName,HIGH,m_nTaskID,this,
                                         objNetInterface,m_objMemPoolRaw,
                                         m_objMemPoolDecodedShort
                                         ,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
            else if(m_emReadoutMode == OPERATION_MODE_24)
                objTask = new LambdaTask(strTaskName,HIGH,m_nTaskID,this,
                                         objNetInterface,m_objMemPoolRaw,
                                         m_objMemPoolDecodedInt,
                                         &m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        }
        else
            objTask = new LambdaTask(strTaskName,HIGH,m_nTaskID,this,
                                     objNetInterface,m_objMemPoolRaw,
                                     m_objMemPoolCompressed,
                                     &m_bstMtxSync,m_vCurrentChip,
                                     m_vNIndex,m_vNNominator,m_nDistortedImageSize);
        m_objThPool->AddTask(objTask);
        m_nTaskID++;
    }

    void LambdaSysImpl::JoinAllDecodeTasks()
    {
        for(int32 i=0;i<m_vNDecodingThreads[0];i++)
            CreateTask(m_strOperationMode,"DecodeImage");
        //Decoders that will run if not receiving images or if frame rates are lower
        for(int32 i=0;i<m_vNDecodingThreads[1];i++)
            CreateTask(m_strOperationMode,"DecodeImage",NORMAL);
        //Additional decoders that will only run if not receiving images
        for(int32 i=0;i<m_vNDecodingThreads[2];i++)
            CreateTask(m_strOperationMode,"DecodeImage",LOW);
    }

    void LambdaSysImpl::StopAllDecodeTasks()
    {
        //in order to reinitialize the decode thread
        m_bSysExit = true;
        while(m_objThPool->GetAvailableThreads()!=m_nThreadNumbers)
            usleep(10000);
        m_bSysExit = false;
    } 
}
