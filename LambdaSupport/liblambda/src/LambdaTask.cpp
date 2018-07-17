/*
 * (c) Copyright 2014-2018 DESY, Yuelong Yu <yuelong.yu@desy.de>
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

#include "LambdaTask.h"
#include "ImageDecoder.h"
#include "DistortionCorrector.h"

namespace DetLambdaNS
{
    //////////////////////////////////////////////////
    /// LambdaMultiLinkUDPRecv
    //////////////////////////////////////////////////
    LambdaTaskMultiLinkUDPRecv::LambdaTaskMultiLinkUDPRecv(string strTaskName,
                                                           Enum_priority Epriority,
                                                           int32 nID,
                                                           NetworkInterface* objNetInt,
                                                           MemPool<char>* objMemPoolRaw)
        :m_objNetInterface(objNetInt),
         m_objMemPoolRaw(objMemPoolRaw)
    {
        LOG_TRACE(__FUNCTION__);

        m_strTaskName = strTaskName;
        m_enumPriority = Epriority;
        m_nID = nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority
        m_fStart = false;


        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
    }

    void LambdaTaskMultiLinkUDPRecv::SetRequestedImages(int32 nImgNo)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_nRequestedImageNo = nImgNo;
    }

    void LambdaTaskMultiLinkUDPRecv::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_strTaskName == "MultiLink")
            DoAcquisitionWithMultiLink();
        else if(m_strTaskName == "MonitorTask")
            DoMonitorListener();

        // m_objSys->GetState(); // When task exits, useful to check state.
    }

    void LambdaTaskMultiLinkUDPRecv::DoMonitorListener()
    {
        LOG_INFOS("listner thread starts" + to_string(m_nID));

        while(true)
        {
            usleep(100);

            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart)
                m_bstCond.wait(lock);

            if(m_fExit)
                break;

            lock.unlock();

            if(m_objMemPoolRaw->GetTotalReceivedFrames() >= m_nRequestedImageNo)
            {
                Stop();
                continue;
            }

            m_objMemPoolRaw->IsImageFinished();

        }
        LOG_INFOS("listner thread exits" + to_string(m_nID));
    }

    void LambdaTaskMultiLinkUDPRecv::DoAcquisitionWithMultiLink()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];
        szt nPacketSize = UDP_PACKET_SIZE_NORMAL;
        int16 shErrorCode = 0;
        int32 lFrameNo = 0;
        int16 shPacketSequenceNo = 0;
        int32 nPos = 0;
        m_objMemPoolRaw->AddTaskFrame(m_nID,-1);

        bool bRollover = false; // Flag for detecting rollover
        int32 nMaxFrameNo = pow(2,24)-1; // Rollover occurs at end of 24-bit counter
        //cout << "Rollover test value" << nMaxFrameNo;

        LOG_INFOS("multi link udp recv starts" + to_string(m_nID));
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart)
            {
                //int32 nCount = 200;
                //int32 nNoDataTimes = 0;

                //read remain data in socket buffer
                m_objNetInterface->ClearDataInSocket();

                m_bstCond.wait(lock);

                shErrorCode = 0;
                lFrameNo = 0;
                shPacketSequenceNo = 0;
                nPos = 0;
                m_objMemPoolRaw->AddTaskFrame(m_nID,-1);
            }

            if(m_fExit)
                break;

            lock.unlock();

            if(m_objMemPoolRaw->GetTotalReceivedFrames() >= m_nRequestedImageNo)
            {
                Stop();
                continue;
            }

            shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);

            if(shErrorCode!=-1)
            {
                lFrameNo = (uchar)ptrchPacket[5]
                    +(uchar)ptrchPacket[4]*256
                    +(uchar)ptrchPacket[3]*256*256;

                if(lFrameNo == nMaxFrameNo)
                {
                    bRollover = true;
                    //cout << "Rollover detected";
                }

                if(bRollover && (lFrameNo < nMaxFrameNo))
                {
                    // In special case of rollover, subsequent images
                    // should have their number increased
                    lFrameNo += (nMaxFrameNo+1);
                }

                shPacketSequenceNo = (uchar)ptrchPacket[2];

                //cout<<"threads recv:"<<m_nID<<"-"<<lFrameNo<<"-"<<shPacketSequenceNo<<endl;

                nPos = (UDP_PACKET_SIZE_NORMAL-UDP_EXTRA_BYTES)*(shPacketSequenceNo-1);
                m_objMemPoolRaw->SetPacket(ptrchPacket,nPos,nPacketSize,
                                           lFrameNo,shErrorCode,m_nID,shPacketSequenceNo);
            }
            else
                usleep(10); // Avoid hitting socket and mutexes too much if no packets available
        }

        delete[] ptrchPacket;
        LOG_INFOS("multi link udp recv exits" + to_string(m_nID));
    }

    //////////////////////////////////////////////////
    /// LambdaSingleLinkUDPRecv
    //////////////////////////////////////////////////
    LambdaTaskSingleLinkUDPRecv::LambdaTaskSingleLinkUDPRecv(string strTaskName,
                                                           Enum_priority Epriority,
                                                           int32 nID,
                                                           NetworkInterface* objNetInt,
                                                           MemPool<char>* objMemPoolRaw)
        :m_objNetInterface(objNetInt),
         m_objMemPoolRaw(objMemPoolRaw)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = strTaskName;
        m_enumPriority = Epriority;
        m_nID = nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority


        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
    }

    void LambdaTaskSingleLinkUDPRecv::SetRequestedImages(int32 nImgNo)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_nRequestedImageNo = nImgNo;
    }

    void LambdaTaskSingleLinkUDPRecv::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        DoAcquisitionWithSingleLink();

        // m_objSys->GetState(); // When task exits, useful to check state.
    }

    void LambdaTaskSingleLinkUDPRecv::DoAcquisitionWithSingleLink()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchTmpImg = new char[m_nRawImageSize];
        int16 shErrorCode = 0;
        int32 nFrameNo = 0;

        LOG_INFOS("single link udp recv starts" + to_string(m_nID));
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart)
            {
                //read remain data in socket buffer
                m_objNetInterface->ClearDataInSocket();

                m_bstCond.wait(lock);

                shErrorCode = 0;
                nFrameNo = 0;
            }

            if(m_fExit)
                break;

            lock.unlock();

            if(nFrameNo >= m_nRequestedImageNo)
            {
                Stop();
                continue;
            }

            shErrorCode = m_objNetInterface->ReceiveData(ptrchTmpImg,m_nRawImageSize);
            nFrameNo++;

            m_objMemPoolRaw->SetImage(ptrchTmpImg,nFrameNo,shErrorCode);
            LOG_INFOS(("Arrived Frame No is:"+to_string(nFrameNo)));
        }

        delete[] ptrchTmpImg;
        LOG_INFOS("single link udp recv exits" + to_string(m_nID));

    }

    //////////////////////////////////////////////////
    /// LambdaSingleLinkTCPRecv
    //////////////////////////////////////////////////
    LambdaTaskSingleLinkTCPRecv::LambdaTaskSingleLinkTCPRecv(string strTaskName,
                                                           Enum_priority Epriority,
                                                           int32 nID,
                                                           NetworkInterface* objNetInt,
                                                           MemPool<char>* objMemPoolRaw)
        :m_objNetInterface(objNetInt),
         m_objMemPoolRaw(objMemPoolRaw)
    {
        LOG_TRACE(__FUNCTION__);

        m_strTaskName = strTaskName;
        m_enumPriority = Epriority;
        m_nID = nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority

        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
    }

    void LambdaTaskSingleLinkTCPRecv::SetRequestedImages(int32 nImgNo)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_nRequestedImageNo = nImgNo;
    }


    void LambdaTaskSingleLinkTCPRecv::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        DoAcquisitionWithSingleLink();

        // m_objSys->GetState(); // When task exits, useful to check state.
    }

    void LambdaTaskSingleLinkTCPRecv::DoAcquisitionWithSingleLink()
    {
        LOG_TRACE(__FUNCTION__);

        //Acquisition with TCP
        //Firstly, need opportunity to break if acq cancelled
        //Secondly, at high data rates packets from consecutive
        //images may be combined - need to handle this carefully
        LOG_TRACE(__FUNCTION__);
        int32 nTCPPacketSize = 1500;
        char* ptrchPacket = new char[nTCPPacketSize];

        size_t nCurrentPacketSize;

        char* ptrchTmpImg = new char[m_nRawImageSize];
        int32 nFrameNo = 0;

        int32 shErrorCode = 0;
        int32 nPacketCode = 0;
        int32 nReceivedData = 0;
        int32 nFrameLength = m_nRawImageSize;
        int32 nBytesLeft = nFrameLength;
        int32 nExcessBytes = 0;

        int32 nCount = 200;
        int32 nNoDataTimes = 0;

        LOG_INFOS("single link tcp recv starts" + to_string(m_nID));

        while(true) // Loop over images
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart)
            {
                //read remain data in socket buffer
                m_objNetInterface->ClearDataInSocket();

                m_bstCond.wait(lock);

                shErrorCode = 0;
                nFrameNo = 0;
                nPacketCode = 0;
                nReceivedData = 0;
                nFrameLength = m_nRawImageSize;
                nBytesLeft = nFrameLength;
                nExcessBytes = 0;
            }

            if(m_fExit)
                break;

            lock.unlock();

            if(nFrameNo >= m_nRequestedImageNo)
            {
                Stop();
                continue;
            }

            // Special case where single packet contains data from 2 images
            if(nExcessBytes > 0)
            {
                std::copy(ptrchPacket+nCurrentPacketSize-nExcessBytes,
                          ptrchPacket+nCurrentPacketSize,ptrchTmpImg+nReceivedData);
                nReceivedData += nExcessBytes;
                nBytesLeft = nBytesLeft-nExcessBytes;
                nExcessBytes = 0;
            }

            //NOTE that in theory we might exit this loop mid-image
            while(true)
            {
                nPacketCode = m_objNetInterface->ReceivePacket(ptrchPacket,nCurrentPacketSize);
                if(nPacketCode==-1)
                {
                    nNoDataTimes++;
                    if(nNoDataTimes>=nCount)
                    {
                        nNoDataTimes = 0;
                        break;
                    }
                }
                else
                {
                    // We have data
                    nNoDataTimes = 0;
                    if(nCurrentPacketSize > static_cast<szt>(nBytesLeft))
                    {
                        //Bytes from next image are present in buffer - need to take note of this
                        std::copy(ptrchPacket,ptrchPacket+nBytesLeft,ptrchTmpImg+nReceivedData);
                        nReceivedData += nBytesLeft;
                        nExcessBytes = nCurrentPacketSize-nBytesLeft;
                        nBytesLeft = 0;
                    }
                    else
                    {
                        std::copy(ptrchPacket,ptrchPacket+nCurrentPacketSize,
                                  ptrchTmpImg+nReceivedData);
                        nReceivedData += nCurrentPacketSize;
                        nBytesLeft = nBytesLeft-(nCurrentPacketSize);
                    }
                }

                if(nBytesLeft==0)
                {
                    //first byte of the image should be 0xa0
                    uchar uchByte = ptrchTmpImg[0];
                    if(uchByte!=0xa0)
                    {
                        LOG_STREAM(__FUNCTION__,ERROR,"Image data is wrong!");
                        shErrorCode = 2;
                    }
                    else
                        shErrorCode = 0;

                    // Make first frame no 1, for consistency with multilink approach
                    nFrameNo++;
                    m_objMemPoolRaw->SetImage(ptrchTmpImg,nFrameNo,shErrorCode);
                    LOG_INFOS(("Arrived Frame No is:"+to_string(nFrameNo)));

                    // Reset some variables
                    nReceivedData = 0;
                    nBytesLeft = nFrameLength;
                    shErrorCode = 0;
                    break;
                }
            }

        }///end of loop

        delete ptrchPacket;
        delete ptrchTmpImg;

        LOG_INFOS("single link tcp recv exits" + to_string(m_nID));
    }
    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage
    //////////////////////////////////////////////////
    LambdaTaskDecodeImage::LambdaTaskDecodeImage()
    :m_pCompressor(nullptr),
     m_fDCEnabled(false),
     m_fCompressionEnabled(false)
    {
        LOG_TRACE(__FUNCTION__);
    }

    LambdaTaskDecodeImage::~LambdaTaskDecodeImage()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_pCompressor)
            m_pCompressor.reset();
    }

    void LambdaTaskDecodeImage::EnableCompression(int16 nType,int16 nCompressionLevel)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        if(m_pCompressor)
            m_pCompressor.reset();

        switch(nType)
        {
            case 1: //defalte
                m_pCompressor
                    = uptr_Compressor(new CompressionContext(
                                          unique_ptr<CompressionInterface>(
                                              new CompressionZlib())));
                m_nCompressionLevel = 2;

                break;
        };

        m_fCompressionEnabled = true;
    }
    void LambdaTaskDecodeImage::EnableCompression(int32 nChunkIn,int32 nChunkOut,int32 nPostCode)
    {
        LOG_TRACE(__FUNCTION__);

        if(m_pCompressor)
            m_pCompressor.reset();
#ifdef ENABLEHWCOMPRESSION
	LOG_INFOS("HW compression is enabled");
        m_pCompressor = uptr_Compressor(new CompressionContext(
                                          unique_ptr<CompressionInterface>(
                                              new CompressionHWAHA(nChunkIn,
                                                                   nChunkOut,
                                                                   static_cast<uint8>(5), // use zlib RFC 1950
                                                                   nPostCode))));
	
#else
	 LOG_STREAM(__FUNCTION__,ERROR,"hw compression cannot be enabled,sw compression is enabled instead");
	 EnableCompression(1,2);
#endif
	 m_fCompressionEnabled = true;
       
    }

    void LambdaTaskDecodeImage::DisableCompression()
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_pCompressor.reset();

        m_fCompressionEnabled = false;
    }

    void LambdaTaskDecodeImage::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        while(true)
        {
            usleep(10);

            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart)
                m_bstCond.wait(lock);

            if(m_fExit)
                break;

            lock.unlock();

            if(*m_enumTargetPriority > m_enumPriority)
            {
                usleep(10000);
                continue;
            }

            if(!DoDecodeImage())
            {
                usleep(100);
                continue;
            }

            if(m_fDCEnabled)
                DoDistortionCorrection();

            SetFirstFrameNo();

            if(m_fCompressionEnabled)
                DoCompression();

            WriteData();
            UpdateLiveImage();
        }
    }

    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage12
    //////////////////////////////////////////////////
    LambdaTaskDecodeImage12::LambdaTaskDecodeImage12(string strTaskName,
                                                     Enum_priority Epriority,
                                                     int32 nID,
                                                     vector<int16> vCurrentChip,
                                                     MemPool<char>* objMemPoolRaw,
                                                     int32 nImageSizeAfterDC)
        :m_vCurrentChip(vCurrentChip),
         m_nDecodedImageSize(nImageSizeAfterDC),
         m_nSubImages(1),
         m_objMemPoolRaw(objMemPoolRaw),
         m_objMemPoolCompressed(nullptr),
         m_objMemPoolDecoded12(nullptr),
         m_pDecoder(new ImageDecoder(m_vCurrentChip)),
         m_nFrameRate(1000)
    {
        LOG_TRACE(__FUNCTION__);

        m_strTaskName = strTaskName;
        m_enumPriority = Epriority;
        m_nID = nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority

        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();


        int32 nXtemp;
        int32 nYtemp;
        m_pDecoder->GetDecodedImageSize(nXtemp, nYtemp);

        m_nImageSizeBeforeDC = nXtemp * nYtemp;
        //m_pLiveImage = new int32[m_nImageSizeBeforeDC];
        //m_p12bitDecodedImg = new int16[m_nImageSizeBeforeDC];

        m_vSrcData.clear();
        m_vSrcData.resize(m_nDecodedImageSize*sizeof(int16));
    }

    LambdaTaskDecodeImage12:: ~LambdaTaskDecodeImage12()
    {
        LOG_TRACE(__FUNCTION__);

        m_pDecoder.reset();

        if(m_pDC16)
            m_pDC16.reset();
    }

    void LambdaTaskDecodeImage12::EnableDC(vector<int32>& vNIndex, vector<int32>& vNNominator)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_vNIndex = vNIndex;
        m_vNNominator = vNNominator;

        m_pDC16 = uptr_DC16(new DistortionCorrector<int16>(m_vNIndex,
                                                           m_vNNominator,
                                                           ((uint32)(2<<12)-1)));
        m_fDCEnabled = true;
    }

    void LambdaTaskDecodeImage12::DisableDC()
    {
        LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_pDC16.reset();
        m_fDCEnabled = false;
    }

    void LambdaTaskDecodeImage12::SetLiveMode(int32 nFrameRate, int32& nFrameNo, int32* pLiveImg)
    {
        LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_nFrameRate = nFrameRate;
        m_nLiveFrameNo = &nFrameNo;
        m_pLiveImage = pLiveImg;
    }

    void LambdaTaskDecodeImage12::SetBuffer(MemPool<char>* objMemPoolCompressed)
    {
        LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_objMemPoolCompressed = objMemPoolCompressed;
    }

    void LambdaTaskDecodeImage12::SetBuffer(MemPool<int16>* objMemPoolDecoded12)
    {
        LOG_TRACE(__FUNCTION__);
        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_objMemPoolDecoded12 = objMemPoolDecoded12;
    }

    bool LambdaTaskDecodeImage12::DoDecodeImage()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchImg = 0;

        if(m_objMemPoolRaw->GetStoredImageNumbers() == 0)
            return false;

        if(m_fCompressionEnabled)
        {
            if(m_objMemPoolCompressed->IsFull())
                return false;
        }
        else
        {
            if(m_objMemPoolDecoded12->IsFull())
                return false;
        }

        if(m_objMemPoolRaw->GetImage(ptrchImg,m_nFrameNo,m_shErrCode,m_nDataLength))
        {
            m_pDecoder->SetRawImage(ptrchImg);
            m_p12bitDecodedImg = m_pDecoder->RunDecodingImg();
            return true;
        }
        return false;
    }

    void LambdaTaskDecodeImage12::DoDistortionCorrection()
    {
        LOG_TRACE(__FUNCTION__);

        int16* pImageAfterDC = m_pDC16->RunDistortCorrect(m_p12bitDecodedImg);
        m_p12bitDecodedImg = pImageAfterDC;
    }

    void LambdaTaskDecodeImage12::DoCompression()
    {
        LOG_TRACE(__FUNCTION__);

        //vector<uchar> vDstData;
		m_vDstData.clear();

        memmove(&m_vSrcData[0],m_p12bitDecodedImg,m_vSrcData.size());

        m_pCompressor->CompressData(m_vSrcData,m_vDstData,m_nCompressionLevel);
        m_pCompressedData = reinterpret_cast<char*>(&m_vDstData[0]);
        m_nDataLength = m_vDstData.size();
    }

    void LambdaTaskDecodeImage12::SetFirstFrameNo()
    {
        if(m_fCompressionEnabled)
        {
            if(m_objMemPoolCompressed->GetFirstFrameNo() == -1)
            {
                int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                //if lFristFrame is -1, means it is single link version
                if(lFristFrame!=-1)
                    m_objMemPoolCompressed->SetFirstFrameNo(lFristFrame);
            }
        }
        else
        {
            if(m_objMemPoolDecoded12->GetFirstFrameNo() == -1)
            {
                int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                //if lFristFrame is -1, means it is single link version
                if(lFristFrame!=-1)
                    m_objMemPoolDecoded12->SetFirstFrameNo(lFristFrame);
            }
        }
    }

    void LambdaTaskDecodeImage12::WriteData()
    {
        if(m_fCompressionEnabled)
            WriteData(m_pCompressedData,m_nFrameNo,m_shErrCode,m_nDataLength);
        else
            WriteData(m_p12bitDecodedImg,m_nFrameNo,m_shErrCode);
    }
    void LambdaTaskDecodeImage12::WriteData(char* pCompressedData,
                                            int32 nFrameNo,
                                            int16 shErrorCode,
                                            int32 nDataSize)
    {
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart || m_fExit)
                break;

            lock.unlock();

            if((m_objMemPoolCompressed->SetImage(pCompressedData,
                                                 nFrameNo,
                                                 shErrorCode,
                                                 true,
                                                 nDataSize)))
                break;
            usleep(50); // Wait for next attempt
        }

    }

    void LambdaTaskDecodeImage12::WriteData(int16* p12bitImg, int32 nFrameNo, int16 shErrorCode)
    {
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart || m_fExit)
                break;

            lock.unlock();

            if(m_objMemPoolDecoded12->SetImage(p12bitImg,
                                               nFrameNo,
                                               shErrorCode,
                                               true))
                break;
            usleep(20); // Wait for next attempt
        }
    }

    void LambdaTaskDecodeImage12::UpdateLiveImage()
    {
        LOG_TRACE(__FUNCTION__);

        if((m_nFrameNo - 1) % m_nFrameRate == 0)
        {
            //cout<<m_nFrameNo<<"-"<<m_nFrameRate<<endl;
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            *m_nLiveFrameNo = m_nFrameNo;
            std::copy(m_p12bitDecodedImg,m_p12bitDecodedImg + m_nDecodedImageSize, m_pLiveImage);
        }
    }

    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage24
    //////////////////////////////////////////////////
    LambdaTaskDecodeImage24::LambdaTaskDecodeImage24(string strTaskName,
                                                     Enum_priority Epriority,
                                                     int32 nID,
                                                     vector<int16> vCurrentChip,
                                                     MemPool<char>* objMemPoolRaw,
                                                     int32 nImageSizeAfterDC)
        :m_vCurrentChip(vCurrentChip),
         m_nDecodedImageSize(nImageSizeAfterDC),
         m_nSubImages(1),
         m_objMemPoolRaw(objMemPoolRaw),
         m_objMemPoolCompressed(nullptr),
         m_objMemPoolDecoded24(nullptr),
         m_pDecoder(new ImageDecoder(m_vCurrentChip)),
         m_nFrameRate(1000)
    {
        LOG_TRACE(__FUNCTION__);

        m_strTaskName = strTaskName;
        m_enumPriority = Epriority;
        m_nID = nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority

        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();


        int32 nXtemp;
        int32 nYtemp;
        m_pDecoder->GetDecodedImageSize(nXtemp, nYtemp);

        m_nImageSizeBeforeDC = nXtemp * nYtemp;
        //m_pLiveImage = new int32[m_nImageSizeBeforeDC];
        m_p24bitDecodedImg = new int32[m_nImageSizeBeforeDC];
        m_p24bitFinalImg = nullptr;

        m_vSrcData.clear();
        m_vSrcData.resize(m_nDecodedImageSize*sizeof(int32));

    }

    LambdaTaskDecodeImage24:: ~LambdaTaskDecodeImage24()
    {
        LOG_TRACE(__FUNCTION__);

        m_pDecoder.reset();

        if(m_pDC32)
            m_pDC32.reset();

        delete[] m_p24bitDecodedImg;

    }

    void LambdaTaskDecodeImage24::EnableDC(vector<int32>& vNIndex, vector<int32>& vNNominator)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_vNIndex = vNIndex;
        m_vNNominator = vNNominator;


        m_pDC32 = uptr_DC32(new DistortionCorrector<int32>(m_vNIndex,
                                                           m_vNNominator,
                                                           ((int32)(2<<24)-1)));
        m_fDCEnabled = true;
    }

    void LambdaTaskDecodeImage24::DisableDC()
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);

        m_pDC32.reset();

        m_fDCEnabled = false;
    }

    void LambdaTaskDecodeImage24::SetLiveMode(int32 nFrameRate, int32& nFrameNo, int32* pLiveImg)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_nFrameRate = nFrameRate;
        m_nLiveFrameNo = &nFrameNo;
        m_pLiveImage = pLiveImg;
    }

    void LambdaTaskDecodeImage24::SetBuffer(MemPool<char>* objMemPoolCompressed)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_objMemPoolCompressed = objMemPoolCompressed;
    }

    void LambdaTaskDecodeImage24::SetBuffer(MemPool<int32>* objMemPoolDecoded24)
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(m_bstSync);
        m_objMemPoolDecoded24 = objMemPoolDecoded24;
    }

    bool LambdaTaskDecodeImage24::DoDecodeImage()
    {
        LOG_TRACE(__FUNCTION__);
        if(m_objMemPoolRaw->GetStoredImageNumbers() < 2)
            return false;

        if(!m_fCompressionEnabled)
        {
            if(m_objMemPoolDecoded24->IsFull())
                return false;
        }
        else
        {
            if(m_objMemPoolCompressed->IsFull())
                return false;
        }
        //if(m_objMemPoolRaw->GetImage())
        /// TODO : get image from raw buffer
        /// run decode image for each image
        char* pImg1 = 0;
        char* pImg2 = 0;
        int32 nFrameNo1 = -1;
        int32 nFrameNo2 = -1;
        int16 sErrCode1 = -1;
        int16 sErrCode2 = -1;
        int16* p12BitImg1;
        int16* p12BitImg2;

        if(m_objMemPoolRaw->Get2Image(pImg1,nFrameNo1,sErrCode1,
                                      pImg2,nFrameNo2,sErrCode2))
        {
            m_pDecoder->SetRawImage(pImg1);
            p12BitImg1 = m_pDecoder->RunDecodingImg();

            m_pDecoder->SetRawImage(pImg2);
            p12BitImg2 = m_pDecoder->RunDecodingImg();

            for(int32 i=0;i<m_nImageSizeBeforeDC;i++)
                m_p24bitDecodedImg[i] = ((int32)p12BitImg1[i])
                    +(((int32)p12BitImg2[i])*4096);

            m_p24bitFinalImg = m_p24bitDecodedImg;

            m_nFrameNo = nFrameNo2/2;
            m_shErrCode = sErrCode1<=sErrCode2?sErrCode1:sErrCode2;

            return true;
        }

        return false;
    }

    void LambdaTaskDecodeImage24::DoDistortionCorrection()
    {
        LOG_TRACE(__FUNCTION__);
        m_p24bitFinalImg = m_pDC32->RunDistortCorrect(m_p24bitDecodedImg);
    }

    void LambdaTaskDecodeImage24::DoCompression()
    {
        LOG_TRACE(__FUNCTION__);
        vector<uchar> vDstData;

        memmove(&m_vSrcData[0],m_p24bitFinalImg,m_vSrcData.size());
        m_pCompressor->CompressData(m_vSrcData,vDstData,m_nCompressionLevel);
        m_pCompressedData = reinterpret_cast<char*>(&vDstData[0]);
        m_nDataLength = vDstData.size();
    }

    void LambdaTaskDecodeImage24::SetFirstFrameNo()
    {
        if(m_fCompressionEnabled)
        {
            if(m_objMemPoolCompressed->GetFirstFrameNo() == -1)
            {
                int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                //if lFristFrame is -1, means it is single link version
                if(lFristFrame!=-1)
                    m_objMemPoolCompressed->SetFirstFrameNo(lFristFrame);
            }
        }
        else
        {
            if(m_objMemPoolDecoded24->GetFirstFrameNo() == -1)
            {
                int32 lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                //if lFristFrame is -1, means it is single link version
                if(lFristFrame!=-1)
                    m_objMemPoolDecoded24->SetFirstFrameNo(lFristFrame);
            }
        }
    }

    void LambdaTaskDecodeImage24::WriteData()
    {
        if(m_fCompressionEnabled)
            WriteData(m_pCompressedData,m_nFrameNo,m_shErrCode,m_nDataLength);
        else
            WriteData(m_p24bitFinalImg,m_nFrameNo,m_shErrCode);
    }

    void LambdaTaskDecodeImage24::WriteData(char* pCompressedData,
                                            int32 nFrameNo,
                                            int16 shErrorCode,
                                            int32 nDataSize)
    {
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart || m_fExit)
                break;

            lock.unlock();

            if((m_objMemPoolCompressed->SetImage(pCompressedData,
                                                 nFrameNo,
                                                 shErrorCode,
                                                 true,
                                                 nDataSize)))
                break;

            usleep(20); // Wait for next attempt
        }

        LOG_INFOS("Write 24bit compressed data finished");
    }

    void LambdaTaskDecodeImage24::WriteData(int32* p24bitImg, int32 nFrameNo, int16 shErrorCode)
    {
        while(true)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            if(!m_fStart || m_fExit)
                break;

            lock.unlock();

            if(m_objMemPoolDecoded24->SetImage(p24bitImg,
                                               nFrameNo,
                                               shErrorCode,
                                               true))
                break;

            usleep(20); // Wait for next attempt
        }

        LOG_INFOS("Write 24bit uncompressed data finished");
    }

    void LambdaTaskDecodeImage24::UpdateLiveImage()
    {
        if((m_nFrameNo - 1) % m_nFrameRate == 0)
        {
            boost::unique_lock<boost::mutex> lock(m_bstSync);
            *m_nLiveFrameNo = m_nFrameNo;
            std::copy(m_p24bitFinalImg,m_p24bitFinalImg + m_nDecodedImageSize, m_pLiveImage);
        }
    }
}
