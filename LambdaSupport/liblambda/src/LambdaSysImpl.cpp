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

#include "LambdaGlobals.h"
#include "LambdaSysImpl.h"
#include "MemUtils.h"
#include "ThreadUtils.h"
#include "NetworkInterface.h"
#include "NetworkImplementation.h"
#include "FilesOperation.h"
#include "LambdaModule.h"
#include "ImageDecoder.h"
#include "LambdaTask.h"
#include "LambdaConfigReader.h"

///namespace DetCommonNS
namespace DetCommonNS
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

        m_strCurrentModuleName = "";
        m_strOperationMode= "";
        m_shTriggerMode = 0;
        m_dShutterTime = 1;
        m_dDelayTime = 0;
        m_lFrameNo = 1000;
        m_bSaveAllImg = false;
        m_bBurstMode = true;
        m_fEnergy = 0;
        //m_nState = 0;
        m_strSaveFilePath = "./";
        //m_strConfigFilePath = "";
        m_nX = -1;
        m_nY = -1;
        m_nImgDepth = -1;
        m_lLastestImgNo = -1;
        m_lImgNo = -1;
        m_lQueueDepth = -1;
        m_lQueueImgNo = -1;
        m_vThreshold.resize(8,300);
        m_objMemPoolDecodedShort = NULL;
        m_objMemPoolDecodedInt = NULL;
        m_objTask = NULL;
        m_nModuleType = 0;
        m_nTaskID = 0;
        m_bRunning = false;
        m_bSysExit = false;
        m_nThreadNumbers = THREAD_NUMBER;
        m_nRawBufferLength = RAW_BUFFER_LENGTH;
        m_nDecodedBufferLength = DECODED_BUFFER_LENGTH;
	m_nDecodingThreads = 3;
	m_nLowPriorityDecodingThreads = 6;

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
        

        SetShutterTime(m_dShutterTime);
        SetTriggerMode(m_shTriggerMode);
        SetNImages(m_lFrameNo);
        SetBurstMode(m_bBurstMode);
    
        for(int i=0;i<m_nDecodingThreads;i++)
            CreateTask(m_strOperationMode,"DecodeImage");
        // Additional decoders that will only run if not receiving images
	for(int i=0;i<m_nLowPriorityDecodingThreads;i++)
	    CreateTask(m_strOperationMode,"DecodeImage",LOW);

        ///quick fix for reading out unsual data
        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];    
        int nPacketSize = UDP_PACKET_SIZE_NORMAL;
        int nCount = 50;
        short shErrorCode = 0;
        int nNoDataTimes = 0;

	usleep(500000); // Wait to ensure bad packets are caught
                    
        for(int i=1;i<m_vNetInterface.size();i++)
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
        delete ptrchPacket;
        ptrchPacket = NULL;

	// Throw in a test of Read OMR
	vector<char> vReadOMRtest;
	vReadOMRtest = m_objLambdaModule->ReadOMR(1);	
                
        m_bRunning = true;
    }

    void LambdaSysImpl::ExitSys()
    {
        LOG_TRACE(__FUNCTION__);
            
        for(int i=0;i<m_vNetInterface.size();i++)
            m_vNetInterface[i]->Disconnect();

        m_bSysExit = true;

        delete m_objMemPoolRaw;
        m_objMemPoolRaw = NULL;

        delete m_objMemPoolDecodedShort;
        m_objMemPoolDecodedShort = NULL;

        delete m_objMemPoolDecodedInt;
        m_objMemPoolDecodedInt = NULL;

        delete m_objThPool;
        m_objThPool = NULL;
        
        m_ptrshDecodeImg = NULL;
    
        m_ptrnDecodeImg = NULL;

        delete m_objConfigReader;
        m_objConfigReader = NULL;

        m_objFileOp = NULL;

        delete m_objLambdaModule;
        m_objLambdaModule = NULL;
       
    }
    
    bool LambdaSysImpl::ReadConfig(bool bSwitchMode, string strOpMode)
    {
        LOG_TRACE(__FUNCTION__);
        
        //destination port No for CH1
        m_objConfigReader->LoadLocalConfig(bSwitchMode, strOpMode);
        m_bMultilink = m_objConfigReader->GetMultilink();
        m_objConfigReader->GetUDPConfig(m_vStrMAC,m_vStrIP,m_vUShPort);
        m_objConfigReader->GetTCPConfig(m_strTCPIPAddress,m_shTCPPortNo);
        m_objConfigReader->GetChipConfig(m_vCurrentChip,m_vStCurrentChipData);
        m_strOperationMode = m_objConfigReader->GetOperationMode();
        m_strCurrentModuleName = m_objConfigReader->GetModuleName();
        m_stDetCfgData = m_objConfigReader->GetDetConfigData();
        m_vfTranslation = m_objConfigReader->GetPosition();

        if(m_nDistortionCorrectionMethod == 0)
        {
            if(m_vCurrentChip.size() == 12)
            {
                m_nX = 1536;
                m_nY = 512;
            }
            else if(m_vCurrentChip.size() == 6)
            {
                m_nX = 768;
                m_nY = 512;
            }
        }
        else if(m_nDistortionCorrectionMethod == 1)
        {
            
            m_objConfigReader->GetDistoredImageSize(m_nDistortedX,m_nDistortedY);
            m_nX = m_nDistortedX;
            m_nY = m_nDistortedY;

            m_vUnPixelMask = m_objConfigReader->GetPixelMask();
            m_vNIndex = m_objConfigReader->GetIndexFile();
            m_vNNominator = m_objConfigReader->GetNominatorFile();
        }

        m_nDistortedImageSize = m_nX*m_nY;

        if(m_strOperationMode == OPERATION_MODE_24)
            m_nImgDepth = 24;
        else 
            m_nImgDepth = 12;

        ///
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
    }
    
    bool LambdaSysImpl::InitNetwork()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_bMultilink)
        {
            //establish TCP link first
            NetworkImplementation* objNetImplTCP = new NetworkTCPImplementation(m_strTCPIPAddress,m_shTCPPortNo);
            NetworkInterface* objNetInterfaceTCP = new NetworkTCPInterface(objNetImplTCP);
            m_vNetInterface.push_back(objNetInterfaceTCP);
            
            ///once TCP is connected successfully, config UDP connections
            if(m_vNetInterface[0]->Connect()==0)
            {
                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,m_vNetInterface[0],m_bMultilink
                                                     ,m_vCurrentChip,m_stDetCfgData,m_vStCurrentChipData);

                //send udp data via TCP
                m_objLambdaModule->WriteUDPMACAddress(0,m_vStrMAC[0]);
                m_objLambdaModule->WriteUDPMACAddress(1,m_vStrMAC[1]);

                m_objLambdaModule->WriteUDPIP(0,m_vStrIP[0]);
                m_objLambdaModule->WriteUDPIP(1,m_vStrIP[1]);
            
                m_objLambdaModule->WriteUDPPorts(0,m_vUShPort);
                m_objLambdaModule->WriteUDPPorts(1,m_vUShPort);
                
                NetworkImplementation* objNetImplUDP = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[1]);
                NetworkImplementation* objNetImplUDP1 = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[2]);
                NetworkImplementation* objNetImplUDP2 = new NetworkUDPImplementation(m_vStrIP[1][0],m_vUShPort[1]);
                NetworkImplementation* objNetImplUDP3 = new NetworkUDPImplementation(m_vStrIP[1][0],m_vUShPort[2]);
            
                NetworkInterface* objNetInterfaceUDP = new NetworkUDPInterface(objNetImplUDP);
                NetworkInterface* objNetInterfaceUDP1 = new NetworkUDPInterface(objNetImplUDP1);
                NetworkInterface* objNetInterfaceUDP2 = new NetworkUDPInterface(objNetImplUDP2);
                NetworkInterface* objNetInterfaceUDP3 = new NetworkUDPInterface(objNetImplUDP3);

                m_vNetInterface.push_back(objNetInterfaceUDP);
                m_vNetInterface.push_back(objNetInterfaceUDP1);
                m_vNetInterface.push_back(objNetInterfaceUDP2);
                m_vNetInterface.push_back(objNetInterfaceUDP3);
                
                for(int i=1;i<m_vNetInterface.size();i++)
                    m_vNetInterface[i]->Connect();
                return true;
            }
            else
                return false;
            
        }
        else
        {
            //init network
            NetworkImplementation* objNetImplTCP = new NetworkTCPImplementation(m_strTCPIPAddress,m_shTCPPortNo);         
            NetworkInterface* objNetInterfaceTCP = new NetworkTCPInterface(objNetImplTCP);
            
            m_vNetInterface.push_back(objNetInterfaceTCP);
            
            if(m_vNetInterface[0]->Connect() == 0)
            {

                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,m_vNetInterface[0],m_bMultilink
                                                     ,m_vCurrentChip,m_stDetCfgData,m_vStCurrentChipData);
                //send udp data via TCP
                m_objLambdaModule->WriteUDPMACAddress(0,m_vStrMAC[0]);
                m_objLambdaModule->WriteUDPIP(0,m_vStrIP[0]);
                m_objLambdaModule->WriteUDPPorts(0,m_vUShPort);
                 
                NetworkImplementation* objNetImplUDP = new NetworkUDPImplementation(m_vStrIP[0][0],m_vUShPort[1]);
                NetworkInterface* objNetInterfaceUDP = new NetworkUDPInterface(objNetImplUDP);
                m_vNetInterface.push_back(objNetInterfaceUDP);
                m_vNetInterface[1]->Connect();
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
        if(m_strOperationMode == OPERATION_MODE_12)
            m_objMemPoolDecodedShort = new MemPool<short>(m_nDecodedBufferLength,m_nDistortedImageSize);
        else if(m_strOperationMode == OPERATION_MODE_24)
            m_objMemPoolDecodedInt = new MemPool<int>(m_nDecodedBufferLength,m_nDistortedImageSize);
        usleep(100);
        m_objMemPoolRaw = new MemPool<char>(m_nRawBufferLength,m_nImgDataSize+UDP_EXTRA_BYTES);
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

    vector<float> LambdaSysImpl::GetPosition()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vfTranslation;
    }
    
        
    void LambdaSysImpl::SetOperationMode(string strOperationMode)
    {
        LOG_TRACE(__FUNCTION__);

        SetState(BUSY);
        if(strOperationMode == OPERATION_MODE_24 || strOperationMode == OPERATION_MODE_12)
        {
            if(m_strOperationMode != strOperationMode && m_bRunning)
            {
                
                //reload configuration files
                m_strOperationMode = strOperationMode;
                ReadConfig(true,m_strOperationMode);
                delete m_objLambdaModule;
                m_objLambdaModule = new LambdaModule(m_strCurrentModuleName,m_vNetInterface[0],m_bMultilink,m_vCurrentChip,m_stDetCfgData,m_vStCurrentChipData);
		
                //cout << "Existing threshold = " << m_nThreshold << " and energy = " << m_fEnergy << endl;
                
                if(m_strOperationMode == OPERATION_MODE_12)
                {
                    delete m_objMemPoolDecodedInt;
                    m_objMemPoolDecodedInt = NULL;
                    m_objMemPoolDecodedShort = new MemPool<short>(m_nDecodedBufferLength,m_nDistortedImageSize);
                    usleep(100);
                }
                
                else if(m_strOperationMode == OPERATION_MODE_24)
                {
                    delete m_objMemPoolDecodedShort;
                    m_objMemPoolDecodedShort = NULL;
                    m_objMemPoolDecodedInt = new MemPool<int>(m_nDecodedBufferLength,m_nDistortedImageSize);
                    usleep(100);
                }
                
                SetThreshold(m_nThreshold, m_fEnergy); 
                SetShutterTime(m_dShutterTime);
                SetTriggerMode(m_shTriggerMode);
                SetNImages(m_lFrameNo);
                SetBurstMode(m_bBurstMode);

                //in order to reinitialize the decode thread
                m_bSysExit = true;
                while(m_objThPool->GetAvailableThreads()!=m_nThreadNumbers)
                    usleep(100);
                cout<<"threads quit"<<endl;

                m_bSysExit = false;
		for(int i=0;i<m_nDecodingThreads;i++)
		  CreateTask(m_strOperationMode,"DecodeImage");
		// Additional decoders that will only run if not receiving images
		for(int i=0;i<m_nLowPriorityDecodingThreads;i++)
		  CreateTask(m_strOperationMode,"DecodeImage",LOW);               
                
            }
        }
        else
            LOG_STREAM(__FUNCTION__,ERROR,("This Operation Mode( "+strOperationMode +") does not exist!"));
        
        SetState(ON);
    }

    string LambdaSysImpl::GetOperationMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strOperationMode;           
    }

    void LambdaSysImpl::SetTriggerMode(short shTriggerMode)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_shTriggerMode = shTriggerMode;
        m_objLambdaModule->WriteTriggerMode(shTriggerMode);
        SetState(ON);
    }

    short LambdaSysImpl::GetTriggerMode()
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

    void LambdaSysImpl::SetNImages(long lImages)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_lFrameNo = lImages;
        m_objLambdaModule->WriteImageNumbers(lImages);                
        SetState(ON);
    }

    long LambdaSysImpl::GetNImages()
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

    void LambdaSysImpl::SetThreshold(int nThresholdNo, float fEnergy)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_nThreshold = nThresholdNo;
        m_vThreshold[nThresholdNo] = fEnergy;
        m_fEnergy = fEnergy;
        m_objLambdaModule->WriteEnergyThreshold(nThresholdNo,fEnergy);
        SetState(ON);
    }

    float LambdaSysImpl::GetThreshold(int nThresholdNo)
    {
        LOG_TRACE(__FUNCTION__);
        m_nThreshold = nThresholdNo;
        m_fEnergy = m_vThreshold[nThresholdNo];
        return m_vThreshold[nThresholdNo];
    }
  
    void LambdaSysImpl::SetState(Enum_detector_state emState)
    {
        LOG_TRACE(__FUNCTION__);
        m_emState = emState;
    }

    Enum_detector_state LambdaSysImpl::GetState()
    {
        //LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstMtxSync); 
        // If imaging is running, need to check on progress here and update if necessary
        if (m_emState >= 4) // States 4 and above are RECEIVING_IMAGES, PROCESSING_IMAGES, FINISHED
        {
            //Check for raw images complete  
            int expectedframes = m_lFrameNo;
            int expectedRawFrames = m_lFrameNo;

            if(m_strOperationMode == OPERATION_MODE_24)
                expectedRawFrames*=2;
                
            
            int rawframes = m_objMemPoolRaw->GetTotalReceivedFrames();

            if(rawframes<expectedRawFrames)
                SetState(RECEIVING_IMAGES);
            else
            {
                int decodedframes = 0;
                if(m_strOperationMode == OPERATION_MODE_12)
                    decodedframes =  m_objMemPoolDecodedShort->GetTotalReceivedFrames();
                else if(m_strOperationMode == OPERATION_MODE_24)
                    decodedframes =  m_objMemPoolDecodedInt->GetTotalReceivedFrames();
                if ((decodedframes < expectedframes)||(GetQueueDepth()>0))
		{
		    if (m_emState != 5) 
		    {
		        SetState(PROCESSING_IMAGES);
			m_objThPool->SetPriorityLevel(LOW); // Allow low priority tasks to run if image collection is done but we have images to process
		    }
		}
                else
		    SetState(FINISHED); // This indicates library done, BUT that we haven't reset state to ON with StopFastImaging.
                // The FINISHED state is to help Tango - it allows the library to remain in a "not ON" state until the Tango server is finished with files etc.
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
	m_objThPool->SetPriorityLevel(NORMAL); // Go to normal priority for task processing

        //cout << "Putting into receiving state \n";
        m_nTaskID = 0;
        //reset once, for avoiding the images come after stopacq command 
        m_objMemPoolRaw->Reset();
        m_objMemPoolRaw->SetRequestedPacketNumber(m_nPacketsNumber);

        if(m_strOperationMode== OPERATION_MODE_12)
            m_objMemPoolDecodedShort->Reset();
        else if (m_strOperationMode== OPERATION_MODE_24)
            m_objMemPoolDecodedInt->Reset();
      

        m_bAcquisitionStart = true;
        m_bAcquisitionStop = false;
        lock.unlock();

        if(!m_bMultilink)
            CreateTask(m_strOperationMode,"StartAcquisition");
        else
        {   
            for(int i=1;i<m_vNetInterface.size();i++)
                CreateTask(m_strOperationMode,"MultiLink",m_vNetInterface[i]);
            
            CreateTask(m_strOperationMode,"MonitorTask"); 
        }

	m_objLambdaModule->StartFastImaging();
    }

    void LambdaSysImpl::StopImaging()
    {
        LOG_TRACE(__FUNCTION__);

        // Only use LambdaModule command to stop imaging if it's currently receiving images - otherwise nothing needs to be done
        if(GetState()==RECEIVING_IMAGES)
            m_objLambdaModule->StopFastImaging();

        boost::unique_lock<boost::mutex> lock(m_bstMtxSync);
        // Do all of this during locked mutex - don't allow state updates during this process for safety!
        m_bAcquisitionStart = false;
        m_bAcquisitionStop = true;
    
        //here reset once, in order to let the decoding thread finishing the rest decoding work
        m_objMemPoolRaw->Reset();
        if(m_strOperationMode == OPERATION_MODE_12)
            m_objMemPoolDecodedShort->Reset();
        else if (m_strOperationMode == OPERATION_MODE_24)
            m_objMemPoolDecodedInt->Reset();
        usleep(100);
        SetState(ON);
    }

    vector<unsigned int> LambdaSysImpl::GetPixelMask()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vUnPixelMask;
    }

    void LambdaSysImpl::SetDistortionCorrecttionMethod(int nMethod)
    {
        LOG_TRACE(__FUNCTION__);

        //new mode
        if(nMethod!=m_nDistortionCorrectionMethod && nMethod<=1 && nMethod>=0 )
        {
            m_nDistortionCorrectionMethod = nMethod;
            ReadConfig(true,m_strOperationMode);
        }
    }

    int LambdaSysImpl::GetDistortionCorrecttionMethod()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nDistortionCorrectionMethod;
    }
        
    void LambdaSysImpl::GetImageFormat(int& nX, int& nY, int& nImgDepth)
    {
        LOG_TRACE(__FUNCTION__);
        nX = m_nX;
        nY = m_nY;
        nImgDepth = m_nImgDepth;
    }
    
    long LambdaSysImpl::GetLatestImageNo()
    {
        LOG_TRACE(__FUNCTION__);
   
        //   m_lLastestImgNo =  m_objMemPoolDecodedShort->GetLastArrivedImageNo();

   
        return m_lLastestImgNo;
    }

    long LambdaSysImpl::GetQueueDepth()
    {
        //LOG_TRACE(__FUNCTION__);

        if(m_strOperationMode == OPERATION_MODE_24)
            m_lQueueDepth =  m_objMemPoolDecodedInt->GetStoredImageNumbers();
        else if(m_strOperationMode == OPERATION_MODE_12)
            m_lQueueDepth =  m_objMemPoolDecodedShort->GetStoredImageNumbers();
        
        if(m_lQueueDepth>0)
            LOG_INFOS(("Currently stored images are:"+to_string(static_cast<long long>(m_lQueueDepth))));
        return m_lQueueDepth;
    }

    string LambdaSysImpl::GetChipID(int chipNo)
    {
        // Use Read OMR functionality to get the OMR value back. This function waits for timeout when called.
        vector<char> v_OMRread = m_objLambdaModule->ReadOMR(chipNo);
	// Current version prints debug info when reading OMR - this is sufficient for tests, but should implement some 
	// Medipix-specific decoding ultimately.
	// For now, just return arbitrary value
	string str_testString = "Testing";
	return str_testString;

    }

    void LambdaSysImpl::GetRawImage(char* ptrchRetImg,long& lFrameNo,short& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
        //m_objMemPoolRaw->GetImage(ptrchRetImg,lFrameNo,shErrCode);
    }

    int* LambdaSysImpl::GetDecodedImageInt(long& lFrameNo, short& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
      
        if(m_objMemPoolDecodedInt->IsImageReadyForReading(1))
        {
            m_objMemPoolDecodedInt->GetImage(m_ptrnDecodeImg,lFrameNo,shErrCode);
            return m_ptrnDecodeImg;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return NULL;
        }
    }
  
    short* LambdaSysImpl::GetDecodedImageShort(long& lFrameNo, short& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
        if(m_objMemPoolDecodedShort->IsImageReadyForReading(1))
        {
            m_objMemPoolDecodedShort->GetImage(m_ptrshDecodeImg,lFrameNo,shErrCode);
            return m_ptrshDecodeImg;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return NULL;
        }

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
        if(m_strOperationMode == OPERATION_MODE_12)
            objTask = new LambdaTask(strTaskName,HIGH,1,this,m_vNetInterface[1],m_objMemPoolRaw,m_objMemPoolDecodedShort,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        else if(m_strOperationMode == OPERATION_MODE_24)
            objTask = new LambdaTask(strTaskName,HIGH,1,this,m_vNetInterface[1],m_objMemPoolRaw,m_objMemPoolDecodedInt,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        
        m_objThPool->AddTask(objTask);
    }

    void LambdaSysImpl::CreateTask(string strOpMode, string strTaskName, Enum_priority enumTaskPriority)
    {
        LOG_TRACE(__FUNCTION__);

        Task* objTask = NULL;
        if(m_strOperationMode == OPERATION_MODE_12)
            objTask = new LambdaTask(strTaskName,enumTaskPriority,1,this,m_vNetInterface[1],m_objMemPoolRaw,m_objMemPoolDecodedShort,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        else if(m_strOperationMode == OPERATION_MODE_24)
            objTask = new LambdaTask(strTaskName,enumTaskPriority,1,this,m_vNetInterface[1],m_objMemPoolRaw,m_objMemPoolDecodedInt,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        
        m_objThPool->AddTask(objTask);
    }


    void LambdaSysImpl::CreateTask(string strOpMode,string strTaskName,NetworkInterface* objNetInterface)
    {      
        Task*  objTask = NULL;
        if(m_strOperationMode == OPERATION_MODE_12)
            objTask = new LambdaTask(strTaskName,HIGH,m_nTaskID,this,objNetInterface,m_objMemPoolRaw,m_objMemPoolDecodedShort,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        else if(m_strOperationMode == OPERATION_MODE_24)
            objTask = new LambdaTask(strTaskName,HIGH,m_nTaskID,this,objNetInterface,m_objMemPoolRaw,m_objMemPoolDecodedInt,&m_bstMtxSync,m_vCurrentChip,m_vNIndex,m_vNNominator);
        
        m_objThPool->AddTask(objTask);
        m_nTaskID++;
    }
}///end of namespace DetCommonNS
