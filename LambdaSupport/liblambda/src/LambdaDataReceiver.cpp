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

#include "LambdaDataReceiver.h"
#include "LambdaTask.h"
#include "LambdaConfigReader.h"
#include "ImageDecoder.h"

namespace DetLambdaNS
{
    //////////////////////////////////////////////////
    /// LambdaDataReceiver
    //////////////////////////////////////////////////
    LambdaDataReceiver::LambdaDataReceiver()
        :m_objMemPoolRaw(nullptr),
         m_objMemPool8bit(nullptr),
         m_ptrImg8bit(nullptr),
         m_ptrLiveImage(nullptr),
         m_vNDecodingThreads({3,3,6}),//HIGH,NORMAL,LOW priority
         m_dCriticalShutterTime(2.1),// critical shutter time in ms
         m_dShutterTime(3000),
         m_nThreadNumbers(THREAD_NUMBER),
         m_nRawBufferLength(RAW_BUFFER_LENGTH),
         m_nDecodedBufferLength(DECODED_BUFFER_LENGTH),
         m_nDistortionCorrectionMethod(1),
         m_nSubimages(1),
         m_nImgFactor(1),
         m_nRequestedImages(0),
         m_nLiveFrameNo(-1),
         m_nChunkIn(DEFAULT_COMPRESSION_CHUNK),
         m_nChunkOut(DEFAULT_COMPRESSION_CHUNK),
         m_nPostCode(0),
         m_bDC(true)
    {
        LOG_TRACE(__FUNCTION__);
    }

    LambdaDataReceiver::~LambdaDataReceiver()
    {
        LOG_TRACE(__FUNCTION__);

        // exit all tasks
        if(!m_objRecvTasks.empty())
            for(auto val : m_objRecvTasks)
            {
                val->Exit();
                usleep(100);
            }

        //delete thread pool
        m_objThPool.reset(nullptr);

        // delete[] m_ptrImg8bit;
        // m_ptrImg8bit = nullptr;

        delete[] m_ptrLiveImage;
        m_ptrLiveImage = nullptr;

        for(auto val : m_vNetInterface)
        {
            val->Disconnect();
            delete val;
        }

        if(m_objMemPoolRaw)
        {
            delete m_objMemPoolRaw;
            m_objMemPoolRaw = nullptr;
        }

        if(m_objMemPool8bit)
        {
            delete m_objMemPool8bit;
            m_objMemPool8bit = nullptr;
        }
    }

    bool LambdaDataReceiver::Init(double dShutter, Enum_readout_mode& mode, LambdaConfigReader& objConfig)
    {
        LOG_TRACE(__FUNCTION__);

        m_dShutterTime = dShutter;
        m_emReadoutMode = mode;

        ReadConfig(objConfig);

        if(!InitNetworkForDataRecv())
            return false;

        InitThreadPool();
        InitMemoryPool();

        CreateRecvTasks();
        CreateImageProcessTasks();

        m_ptrLiveImage = new int32[m_nDistortedImageSize];
        UpdateLiveMode();

        return true;
    }

    void LambdaDataReceiver::Start()
    {
        LOG_TRACE(__FUNCTION__);

        Reset();

        for(auto val : m_objRecvTasks)
            val->SetRequestedImages(m_nRequestedImages);
    }

    void LambdaDataReceiver::Stop()
    {
        LOG_TRACE(__FUNCTION__);

        Reset();
    }

    void LambdaDataReceiver::SetPriorityLevel(Enum_priority newPriority)
    {
        LOG_TRACE(__FUNCTION__);

        m_objThPool->SetPriorityLevel(newPriority);
    }

    void LambdaDataReceiver::SetRequestedImages(int32 nImgs)
    {
        LOG_TRACE(__FUNCTION__);

        m_nRequestedImages = nImgs*m_nImgFactor;
    }

    bool LambdaDataReceiver::EnableCompression(int32 nMethod, int32 nCompressionRatio)
    {
        LOG_TRACE(__FUNCTION__);

        /// TODO check valid compression method
        if(nMethod <= 0)
            return false;

        m_nCompressionMethod = nMethod;
        m_bCompressionEnabled = true;

        UpdateBuffer();
        UpdateTasks();

        return true;
    }

    void LambdaDataReceiver::DisableCompression()
    {
        LOG_TRACE(__FUNCTION__);

        m_bCompressionEnabled = false;
        m_nCompressionMethod = 0;

        UpdateBuffer();
        UpdateTasks();
    }

    bool LambdaDataReceiver::EnableDistortionCorrection(int32 nMethod)
    {
        LOG_TRACE(__FUNCTION__);

        if(nMethod != 1) // only 1 is supported currently
            return false;

        m_nDistortionCorrectionMethod = nMethod;
        m_bDC = true;

        UpdateBuffer();
        UpdateTasks();

        return true;
    }

    void LambdaDataReceiver::DisableDistiotionCorrection()
    {
        LOG_TRACE(__FUNCTION__);

        m_bDC = false;
        m_nDistortionCorrectionMethod = 0;

        UpdateBuffer();
        UpdateTasks();
    }

    void LambdaDataReceiver::GetImageInfo(int32& nX, int32& nY, int32& nImgDepth)
    {
        LOG_TRACE(__FUNCTION__);

        nX = m_nX;
        nY = m_nY;
        nImgDepth = m_nImgDepth;
    }

    int32 LambdaDataReceiver::GetSubimages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nSubimages;
    }

    int32 LambdaDataReceiver::GetFreeBufferSize()
    {
        LOG_TRACE(__FUNCTION__);

        return (m_nRawBufferLength - (m_objMemPoolRaw->GetStoredImageNumbers()));
    }

    int32 LambdaDataReceiver::GetExpectedDecodedImages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nRequestedImages/m_nImgFactor;
    }

    int32 LambdaDataReceiver::GetExpectedRawImages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nRequestedImages;
    }

    int32 LambdaDataReceiver::GetReceivedRawImages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objMemPoolRaw->GetTotalReceivedFrames();
    }

    void LambdaDataReceiver::SetShutterTime(double dShutterTime)
    {
        LOG_TRACE(__FUNCTION__);

        m_dShutterTime = dShutterTime;
        UpdateLiveMode();
    }

    int32 LambdaDataReceiver::GetQueueDepth()
    {
        LOG_TRACE(__FUNCTION__);

        return -1;
    }

    int32* LambdaDataReceiver::GetCurrentImage(int32& lFrameNo,int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        lFrameNo = m_nLiveFrameNo;
        shErrCode = 0;

        return m_ptrLiveImage;
    }

    char* LambdaDataReceiver::GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength)
    {
        LOG_TRACE(__FUNCTION__);

        if(m_objMemPool8bit->IsImageReadyForReading(1))
        {
            m_objMemPool8bit->GetImage(m_ptrImg8bit,lFrameNo,shErrCode,nDataLength);
            return m_ptrImg8bit;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            nDataLength = -1;
            return nullptr;
        }
    }

    int16* LambdaDataReceiver::GetDecodedImageShort(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        lFrameNo = -1;
        shErrCode = -1;
        return nullptr;
    }

    int32* LambdaDataReceiver::GetDecodedImageInt(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        lFrameNo = -1;
        shErrCode = -1;
        return nullptr;
    }

    void LambdaDataReceiver::InitThreadPool()
    {
        LOG_TRACE(__FUNCTION__);

        //init thread pool
        m_objThPool = uptr_tp(new ThreadPool(m_nThreadNumbers));
        usleep(100);
    }

    void LambdaDataReceiver::InitMemoryPool()
    {
        LOG_TRACE(__FUNCTION__);

        //init memory pool raw images
        m_objMemPoolRaw = new MemPool<char>(m_nRawBufferLength,m_nImgDataSize);
        usleep(100);
    }

    bool LambdaDataReceiver::InitNetworkForDataRecv()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_bMultilink)
        {
            NetworkImplementation* objNetImplUDP
                = new NetworkUDPImplementation(m_vStrIPs[0][0],m_usPorts[1]);
            NetworkImplementation* objNetImplUDP1
                = new NetworkUDPImplementation(m_vStrIPs[0][0],m_usPorts[2]);
            NetworkImplementation* objNetImplUDP2
                = new NetworkUDPImplementation(m_vStrIPs[1][0],m_usPorts[1]);
            NetworkImplementation* objNetImplUDP3
                = new NetworkUDPImplementation(m_vStrIPs[1][0],m_usPorts[2]);

            NetworkInterface* objNetInterfaceUDP = new NetworkUDPInterface(objNetImplUDP);
            NetworkInterface* objNetInterfaceUDP1 = new NetworkUDPInterface(objNetImplUDP1);
            NetworkInterface* objNetInterfaceUDP2 = new NetworkUDPInterface(objNetImplUDP2);
            NetworkInterface* objNetInterfaceUDP3 = new NetworkUDPInterface(objNetImplUDP3);

            m_vNetInterface.push_back(objNetInterfaceUDP);
            m_vNetInterface.push_back(objNetInterfaceUDP1);
            m_vNetInterface.push_back(objNetInterfaceUDP2);
            m_vNetInterface.push_back(objNetInterfaceUDP3);
        }
        else
        {
            // burst mode
            // udp is used
            if(m_bBurstMode)
            {
                NetworkImplementation* objNetImplData
                    = new NetworkUDPImplementation(m_vStrIPs[0][0],m_usPorts[1]);
                NetworkInterface* objNetInterfaceData
                    = new NetworkUDPInterface(objNetImplData);
                m_vNetInterface.push_back(objNetInterfaceData);
            }
            else // use tcp for data transmission
            {
                //Use TCP interface - for now, have "magic number" 3490
                //as port but should implement more flexibly
                NetworkImplementation* objNetImplData
                    = new NetworkTCPImplementation(m_strTCPIPAddress,3490);
                NetworkInterface* objNetInterfaceData
                    = new NetworkTCPInterface(objNetImplData);
                m_vNetInterface.push_back(objNetInterfaceData);
            }
        }

        // connect network
        for(auto val : m_vNetInterface)
            if(val->Connect() != 0)
                return false;

        return true;
    }

    void LambdaDataReceiver::ReadConfig(LambdaConfigReader& objConfig)
    {
        LOG_TRACE(__FUNCTION__);

        m_bMultilink = objConfig.GetMultilink();
        m_bBurstMode = objConfig.GetBurstMode();
        objConfig.GetUDPConfig(m_vStrMACs,m_vStrIPs,m_usPorts);
        objConfig.GetTCPConfig(m_strTCPIPAddress,m_shTCPPortNo);
        objConfig.GetChipConfig(m_vCurrentChip,m_vStCurrentChipData);
        objConfig.GetCompressionChunkConfig(m_nChunkIn,m_nChunkOut,m_nPostCode);

        m_nRawBufferLength = objConfig.GetRawBufferLength();
        m_nDecodedBufferLength = objConfig.GetDecodedBufferLength();
        m_vNDecodingThreads = objConfig.GetDecodingThreadNumbers();
        m_dCriticalShutterTime = objConfig.GetCriticalShutterTime();

        if(m_nDistortionCorrectionMethod == 0)
        {
            // Temporarily create image decoder, to conveniently find decoded image size
            ImageDecoder* objTempImageDecoder = new ImageDecoder(m_vCurrentChip);
            objTempImageDecoder->GetDecodedImageSize(m_nX, m_nY);
            delete objTempImageDecoder;
        }

        if(m_nDistortionCorrectionMethod == 1)
        {
            objConfig.GetDistoredImageSize(m_nDistortedX,m_nDistortedY);
            if((m_nDistortedX > 0) && (m_nDistortedY > 0))
            {
                m_nX = m_nDistortedX;
                m_nY = m_nDistortedY;

                m_vUnPixelMask = objConfig.GetPixelMask();
                m_vNIndex = objConfig.GetIndexFile();
                m_vNNominator = objConfig.GetNominatorFile();
            }
            else
            {
                m_nDistortionCorrectionMethod = 0;
                m_bDC = false;
            }
        }
        m_nDistortedImageSize = m_nX*m_nY;

        // // Have 2 thresholds in 2x12 mode currently
        // if(m_emReadoutMode == OPERATION_MODE_2x12)
        //     m_nSubimages = 2;

        // Calculate the image size and no of packets based on the no of chips
        // Note - for 1-chip tests, might need to "force" the image size to
        // simplify firmware tests (e.g. test 1 chip without having to change data output)
        m_nImgDataSize = m_vCurrentChip.size()  * (BYTES_IN_CHIP  + CHIP_HEADER_SIZE);
        m_nPacketsNumber = m_nImgDataSize / (UDP_PACKET_SIZE_NORMAL - UDP_EXTRA_BYTES);

        // No of packets is total data divided by data per packet, rounded up
        if((m_nImgDataSize % (UDP_PACKET_SIZE_NORMAL - UDP_EXTRA_BYTES)) != 0)
            m_nPacketsNumber++;
    }

    void LambdaDataReceiver::CreateRecvTasks()
    {
        LambdaRecvTask* objTask;
        int32 nTaskID = 0;

        // udp
        // multilink
        if(m_bMultilink)
        {
            // create data receiver task
            for(auto val : m_vNetInterface)
            {
                objTask = new LambdaTaskMultiLinkUDPRecv("MultiLink",HIGH,nTaskID,
                                                          val,m_objMemPoolRaw);

                m_objRecvTasks.push_back(objTask);
                m_objThPool->AddTask(objTask);

                nTaskID++;
            }

            // create additional monitor task
            objTask = new LambdaTaskMultiLinkUDPRecv("MonitorTask",HIGH,nTaskID,
                                                     nullptr,m_objMemPoolRaw);

            m_objRecvTasks.push_back(objTask);
            m_objThPool->AddTask(objTask);
        }
        else
        {
            // burst mode
            // udp is used
            if(m_bBurstMode)
            {
                objTask = new LambdaTaskSingleLinkUDPRecv("SingleLinkUDP",HIGH,nTaskID,
                                                      m_vNetInterface[0],m_objMemPoolRaw);

                m_objRecvTasks.push_back(objTask);
                m_objThPool->AddTask(objTask);
            }
            else // use tcp for data transmission
            {
                objTask = new LambdaTaskSingleLinkTCPRecv("SingleLinkTCP",HIGH,nTaskID,
                                                      m_vNetInterface[0],m_objMemPoolRaw);

                m_objRecvTasks.push_back(objTask);
                m_objThPool->AddTask(objTask);
            }
        }
    }

    void LambdaDataReceiver::Reset()
    {
        LOG_TRACE(__FUNCTION__);

        m_objMemPoolRaw->Reset();
        m_objMemPoolRaw->SetRequestedPacketNumber(m_nPacketsNumber);

        m_objThPool->SetPriorityLevel(HIGH);

        if(m_dShutterTime > m_dCriticalShutterTime)
            m_objThPool->SetPriorityLevel(NORMAL);

        usleep(100);
    }

    //////////////////////////////////////////////////
    /// LambdaDataReceiver12BitMode OPERATION_MODE_12
    //////////////////////////////////////////////////
    LambdaDataReceiver12BitMode::LambdaDataReceiver12BitMode()
        :m_objMemPool16bit(nullptr),
         m_ptrImg16bit(nullptr)
    {
        LOG_TRACE(__FUNCTION__);

        m_nSubimages = 1;
        m_nImgFactor = 1;
        m_nImgDepth = 12;
    }

    LambdaDataReceiver12BitMode::~LambdaDataReceiver12BitMode()
    {
        LOG_TRACE(__FUNCTION__);

        for(auto val : m_objImageProcessTasks)
        {
            val->Exit();
            usleep(100);
        }

        if(m_objMemPool16bit)
        {
            delete m_objMemPool16bit;
            m_objMemPool16bit = nullptr;
        }
    }

    bool LambdaDataReceiver12BitMode::Init(double dShutter,
                                           Enum_readout_mode& mode,
                                           LambdaConfigReader& objConfig)
    {
        LOG_TRACE(__FUNCTION__);

        if(!LambdaDataReceiver::Init(dShutter,mode,objConfig))
            return false;

        this->InitMemoryPool();

        UpdateTasks();
        return true;
    }

    void LambdaDataReceiver12BitMode::Start()
    {
        LOG_TRACE(__FUNCTION__);

        LambdaDataReceiver::Start();

        if(m_bCompressionEnabled)
            m_objMemPool8bit->Reset();
        else
            m_objMemPool16bit->Reset();

        // start image process tasks
        for(auto val : m_objImageProcessTasks)
            val->Start();

        //  start recv tasks
        for(auto val : m_objRecvTasks)
            val->Start();
    }

    void LambdaDataReceiver12BitMode::Stop()
    {
        // stop recv tasks
        for(auto val : m_objRecvTasks)
            val->Stop();

        // stop image process tasks
        for(auto val : m_objImageProcessTasks)
            val->Stop();

        if(m_bCompressionEnabled)
            m_objMemPool8bit->Reset();
        else
            m_objMemPool16bit->Reset();

        LambdaDataReceiver::Stop();
        usleep(100);
    }

    int32 LambdaDataReceiver12BitMode::GetReceivedDecodedImages()
    {
        LOG_TRACE(__FUNCTION__);

        return GetReceivedImages();
    }

    int32 LambdaDataReceiver12BitMode::GetQueueDepth()
    {
        if(m_bCompressionEnabled)
            return m_objMemPool8bit->GetStoredImageNumbers();
        else
            return m_objMemPool16bit->GetStoredImageNumbers();
    }

    int16* LambdaDataReceiver12BitMode::GetDecodedImageShort(int32& lFrameNo, int16& shErrCode)
    {
        int32 nDataLength;

        if(m_objMemPool16bit->IsImageReadyForReading(1))
        {
            m_objMemPool16bit->GetImage(m_ptrImg16bit,lFrameNo,shErrCode,nDataLength);
            return m_ptrImg16bit;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return nullptr;
        }
    }

    void LambdaDataReceiver12BitMode::InitMemoryPool()
    {
        m_objMemPool16bit
            = new MemPool<int16>(m_nDecodedBufferLength,m_nDistortedImageSize);
    }

    void LambdaDataReceiver12BitMode::CreateImageProcessTasks()
    {
        LambdaTaskDecodeImage12* objTask;
        for(int32 i=0;i<m_vNDecodingThreads[1];i++)
        {
            objTask = new LambdaTaskDecodeImage12("DecodeImage",HIGH,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);

        }
        //Decoders that will run if not receiving images or if frame rates are lower
        for(int32 i=0;i<m_vNDecodingThreads[1];i++)
        {
            objTask = new LambdaTaskDecodeImage12("DecodeImage",NORMAL,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);
        }
        //Additional decoders that will only run if not receiving images
        for(int32 i=0;i<m_vNDecodingThreads[2];i++)
        {
            objTask = new LambdaTaskDecodeImage12("DecodeImage",LOW,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);
        }

        for(auto val : m_objImageProcessTasks)
            m_objThPool->AddTask(val);
    }

    int32 LambdaDataReceiver12BitMode::GetReceivedImages()
    {
        if(m_bCompressionEnabled)
            return m_objMemPool8bit->GetTotalReceivedFrames();
        else
            return m_objMemPool16bit->GetTotalReceivedFrames();
    }

    void LambdaDataReceiver12BitMode::UpdateBuffer()
    {
        if(!m_bCompressionEnabled) // no Compression
        {
            if(m_objMemPool16bit->GetElementSize() != static_cast<szt>(m_nDistortedImageSize))
            {
                delete m_objMemPool16bit;
                m_objMemPool16bit = nullptr;

                InitMemoryPool();
            }
        }
        else // with Compression
        {
            if(!m_objMemPool8bit)
            {
                m_objMemPool8bit
                    = new MemPool<char>(m_nDecodedBufferLength,m_nDistortedImageSize*3);
            }
            else
            {
                if(m_objMemPool8bit->GetElementSize() != static_cast<szt>(m_nDistortedImageSize*3))
                {
                    delete m_objMemPool8bit;
                    m_objMemPool8bit = nullptr;

                    m_objMemPool8bit
                        = new MemPool<char>(m_nDecodedBufferLength,m_nDistortedImageSize*3);
                }
            }

        }
    }

    void LambdaDataReceiver12BitMode::UpdateTasks()
    {
        // stop image process tasks
        for(auto val : m_objImageProcessTasks)
        {
            if(!m_bDC)
                val->DisableDC();
            else
                val->EnableDC(m_vNIndex, m_vNNominator);

            if(!m_bCompressionEnabled)
            {
                val->DisableCompression();
                val->SetBuffer(m_objMemPool16bit);
            }
            else
            {
                switch(m_nCompressionMethod)
                {
                    case 1:
                        val->EnableCompression(m_nCompressionMethod);
                        break;
                    case 2: // hardware compression
                        val->EnableCompression(m_nChunkIn,m_nChunkOut,m_nPostCode);
                        break;
                }

                val->SetBuffer(m_objMemPool8bit);
            }
        }
    }

    void LambdaDataReceiver12BitMode::UpdateLiveMode()
    {
        LOG_TRACE(__FUNCTION__);
        for(auto val : m_objImageProcessTasks)
        {
            // limit frame rate to 10Hz
            int32 nFrameRate = (int32)(100/m_dShutterTime);
            nFrameRate = (nFrameRate == 0? 1 : nFrameRate);
            val->SetLiveMode(nFrameRate,m_nLiveFrameNo,m_ptrLiveImage);
        }
    }

    //////////////////////////////////////////////////
    /// LambdaDataReceiver24BitMode OPERATION_MODE_24
    //////////////////////////////////////////////////
    LambdaDataReceiver24BitMode::LambdaDataReceiver24BitMode()
        :m_objMemPool32bit(nullptr),
         m_ptrImg32bit(nullptr)
    {
        m_nSubimages = 1;
        m_nImgFactor = 2;
        m_nImgDepth = 24;
    }

    LambdaDataReceiver24BitMode::~LambdaDataReceiver24BitMode()
    {
        for(auto val : m_objImageProcessTasks)
        {
            val->Exit();
            usleep(100);
        }

        if(m_objMemPool32bit)
        {
            delete m_objMemPool32bit;
            m_objMemPool32bit = nullptr;
        }
    }

    bool LambdaDataReceiver24BitMode::Init(double dShutter,
                                           Enum_readout_mode& mode,
                                           LambdaConfigReader& objConfig)
    {
        if(!LambdaDataReceiver::Init(dShutter,mode,objConfig))
            return false;

        this->InitMemoryPool();

        UpdateTasks();

        return true;
    }

    void LambdaDataReceiver24BitMode::Start()
    {
        LambdaDataReceiver::Start();

        if(m_bCompressionEnabled)
            m_objMemPool8bit->Reset();
        else
            m_objMemPool32bit->Reset();

        // start image process tasks
        for(auto val : m_objImageProcessTasks)
            val->Start();

        //  start recv tasks
        for(auto val : m_objRecvTasks)
            val->Start();
    }

    void LambdaDataReceiver24BitMode::Stop()
    {
        // stop recv tasks
        //  start recv tasks
        for(auto val : m_objRecvTasks)
            val->Stop();
        // stop image process tasks

        for(auto val : m_objImageProcessTasks)
            val->Stop();

        if(m_bCompressionEnabled)
            m_objMemPool8bit->Reset();
        else
            m_objMemPool32bit->Reset();

        LambdaDataReceiver::Stop();

        usleep(100);
    }

    int32 LambdaDataReceiver24BitMode::GetReceivedDecodedImages()
    {
        return GetReceivedImages();
    }

    int32 LambdaDataReceiver24BitMode::GetQueueDepth()
    {
        if(m_bCompressionEnabled)
            return m_objMemPool8bit->GetStoredImageNumbers();
        else
            return m_objMemPool32bit->GetStoredImageNumbers();
    }

    int32* LambdaDataReceiver24BitMode::GetDecodedImageInt(int32& lFrameNo, int16& shErrCode)
    {
        //cout<<"get decoded data 32bit"<<endl;
        int32 nDataLength;

        if(m_objMemPool32bit->IsImageReadyForReading(1))
        {   
            m_objMemPool32bit->GetImage(m_ptrImg32bit,lFrameNo,shErrCode,nDataLength);
            return m_ptrImg32bit;
        }
        else
        {
            lFrameNo = -1;
            shErrCode = -1;
            return nullptr;
        }
    }

    void LambdaDataReceiver24BitMode::InitMemoryPool()
    {
        m_objMemPool32bit
            = new MemPool<int32>(m_nDecodedBufferLength,m_nDistortedImageSize);
    }

    void LambdaDataReceiver24BitMode::CreateImageProcessTasks()
    {
        LambdaTaskDecodeImage24* objTask;
        for(int32 i=0;i<m_vNDecodingThreads[1];i++)
        {
            objTask = new LambdaTaskDecodeImage24("DecodeImage",HIGH,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);

        }
        //Decoders that will run if not receiving images or if frame rates are lower
        for(int32 i=0;i<m_vNDecodingThreads[1];i++)
        {
            objTask = new LambdaTaskDecodeImage24("DecodeImage",NORMAL,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);
        }
        //Additional decoders that will only run if not receiving images
        for(int32 i=0;i<m_vNDecodingThreads[2];i++)
        {
            objTask = new LambdaTaskDecodeImage24("DecodeImage",LOW,1,
                                                  m_vCurrentChip,
                                                  m_objMemPoolRaw,
                                                  m_nDistortedImageSize);

            m_objImageProcessTasks.push_back(objTask);
        }

        for(auto val : m_objImageProcessTasks)
            m_objThPool->AddTask(val);
    }

    int32 LambdaDataReceiver24BitMode::GetReceivedImages()
    {
        if(m_bCompressionEnabled)
            return m_objMemPool8bit->GetTotalReceivedFrames();
        else
            return m_objMemPool32bit->GetTotalReceivedFrames();
    }

    void LambdaDataReceiver24BitMode::UpdateBuffer()
    {
        if(!m_bCompressionEnabled) //no Compression
        {
            if(m_objMemPool32bit->GetElementSize() != static_cast<szt>(m_nDistortedImageSize))
            {
                delete m_objMemPool32bit;
                m_objMemPool32bit = nullptr;

                InitMemoryPool();
            }
        }
        else // with compression
        {
            if(!m_objMemPool8bit)
            {
                m_objMemPool8bit
                    = new MemPool<char>(m_nDecodedBufferLength,m_nDistortedImageSize*5);
            }
            else
            {
                if(m_objMemPool8bit->GetElementSize() != static_cast<szt>(m_nDistortedImageSize*5))
                {
                    delete m_objMemPool8bit;
                    m_objMemPool8bit = nullptr;

                    m_objMemPool8bit
                        = new MemPool<char>(m_nDecodedBufferLength,m_nDistortedImageSize*5);
                }
            }
        }
    }

    void LambdaDataReceiver24BitMode::UpdateTasks()
    {
        // stop image process tasks
        for(auto val : m_objImageProcessTasks)
        {
            if(!m_bDC)
                val->DisableDC();
            else
                val->EnableDC(m_vNIndex, m_vNNominator);

            if(!m_bCompressionEnabled)
            {
                val->DisableCompression();
                val->SetBuffer(m_objMemPool32bit);
            }
            else
            {
                val->EnableCompression(m_nCompressionMethod);
                val->SetBuffer(m_objMemPool8bit);
            }

            // limit frame rate to 10Hz
            int32 nFrameRate = (int32)(100/m_dShutterTime);
            nFrameRate = (nFrameRate == 0? 1 : nFrameRate);
            val->SetLiveMode(nFrameRate,m_nLiveFrameNo,m_ptrLiveImage);
        }
    }

    void LambdaDataReceiver24BitMode::UpdateLiveMode()
    {
        LOG_TRACE(__FUNCTION__);
        for(auto val : m_objImageProcessTasks)
        {
            // limit frame rate to 10Hz
            int32 nFrameRate = (int32)(100/m_dShutterTime);
            nFrameRate = (nFrameRate == 0? 1 : nFrameRate);
            val->SetLiveMode(nFrameRate,m_nLiveFrameNo,m_ptrLiveImage);
        }
    }
}
