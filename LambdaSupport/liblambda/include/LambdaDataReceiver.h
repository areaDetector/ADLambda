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
#include "CompressionContext.h"
#include "Compression.h"

#include <fsdetector/core/NetworkInterface.h>
#include <fsdetector/core/NetworkImplementation.h>
#include <fsdetector/core/Utils.h>
#include <fsdetector/core/FilesOperation.h>
#include <fsdetector/core/ThreadUtils.h>
#include <fsdetector/core/MemUtils.h>

///namespace DetLambdaNS
namespace DetLambdaNS
{
    using namespace FSDetCoreNS;

    class LambdaRecvTask;
    class LambdaTaskDecodeImage;

    class LambdaConfigReader;

    typedef unique_ptr<ThreadPool> uptr_tp;

    //////////////////////////////////////////////////
    /// LambdaDataReceiver Base Class
    //////////////////////////////////////////////////
    /**
     * @brief lambda data receiver
     *        include data receiver and image process
     */
    class LambdaDataReceiver
    {
    public:
        LambdaDataReceiver();
        virtual ~LambdaDataReceiver();

        void SetPriorityLevel(Enum_priority newPriority);
        void SetRequestedImages(int32 nImgs);
        int32 GetSubimages();
        int32 GetExpectedDecodedImages();
        int32 GetExpectedRawImages();
        int32 GetReceivedRawImages();
        void SetShutterTime(double dShutterTime);

        virtual bool Init(double dShutter,Enum_readout_mode& mode, LambdaConfigReader& objConfig);
        virtual void Start();
        virtual void Stop();

        bool EnableCompression(int32 nMethod,int32 nComprssionRatio);
        void DisableCompression();

        bool EnableDistortionCorrection(int32 nMethod);
        void DisableDistiotionCorrection();

        void GetImageInfo(int32& nX, int32& nY, int32& nImgDepth);
        virtual int32 GetFreeBufferSize();

        virtual int32 GetQueueDepth();
        int32* GetCurrentImage(int32& lFrameNo,int16& shErrCode);

        char* GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength);
        virtual int16* GetDecodedImageShort(int32& lFrameNo, int16& shErrCode);
        virtual int32* GetDecodedImageInt(int32& lFrameNo, int16& shErrCode);

        virtual int32 GetReceivedDecodedImages() = 0;

    protected:
        void InitMemoryPool();
        bool InitNetworkForDataRecv();
        virtual void CreateImageProcessTasks() = 0;
        virtual void UpdateTasks() = 0;
        virtual void UpdateBuffer() = 0;
        virtual void UpdateLiveMode() = 0;

        void InitThreadPool();
        void ReadConfig(LambdaConfigReader& objConfig);
        void CreateRecvTasks();
        void Reset();

        /// member variables
        uptr_tp m_objThPool;

        MemPool<char>*  m_objMemPoolRaw;
        MemPool<char>*  m_objMemPool8bit;

        vector<LambdaRecvTask*> m_objRecvTasks;
        vector<LambdaTaskDecodeImage*> m_objImageProcessTasks;
        vector<NetworkInterface*> m_vNetInterface;
        char* m_ptrImg8bit;
        int32* m_ptrLiveImage;

        // setting of decoding threads
        vector<int32> m_vNDecodingThreads;

        vector<int16> m_vCurrentChip;
        vector<stMedipixChipData> m_vStCurrentChipData;

        vector<uint32> m_vUnPixelMask;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;
        vector<float> m_vfTranslation;

        // data receiver IPs and ports
        vector< vector<string> > m_vStrMACs;
        vector<vector<string>> m_vStrIPs;
        vector<uint16> m_usPorts;

        // critical time
        // if shutter time is smaller than this time
        // only the HIGH priority threads will run
        // during data receiving
        double m_dCriticalShutterTime,m_dShutterTime;

        int32 m_nThreadNumbers,
            m_nRawBufferLength,
            m_nDecodedBufferLength,
            m_nDecodingThreads,
            m_nDistortionCorrectionMethod,
            m_nX,
            m_nY,
            m_nImgDepth,
            m_nDistortedX,
            m_nDistortedY,
            m_nDistortedImageSize,
            m_nSubimages,
            m_nImgDataSize,
            m_nPacketsNumber,
            m_nCompressionRatio,
            m_nCompressionMethod,
            m_nImgFactor,
            m_nRequestedImages,
            m_nTotalImages,
            m_nLiveFrameNo,
            m_nChunkIn,
            m_nChunkOut,
            m_nPostCode;

        int16 m_shTCPPortNo;

        bool m_bBurstMode,m_bMultilink,m_bSlaveModule,m_bCompressionEnabled,m_bDC;

        Enum_readout_mode m_emReadoutMode;
        string m_strTCPIPAddress;

    };

    //////////////////////////////////////////////////
    /// LambdaDataReceiver12BitMode OPERATION_MODE_12
    //////////////////////////////////////////////////
    /**
     * @biref data receiver for 12bit mode
     */
    class LambdaDataReceiver12BitMode : public LambdaDataReceiver
    {
    public:
        LambdaDataReceiver12BitMode();
        ~LambdaDataReceiver12BitMode();

        bool Init(double dShutter,Enum_readout_mode& mode, LambdaConfigReader& objConfig) override;
        void Start() override;
        void Stop() override;

        int32 GetReceivedDecodedImages() override;
        int32 GetQueueDepth() override;

        int16* GetDecodedImageShort(int32& lFrameNo, int16& shErrCode) override;

    private:
        void InitMemoryPool();
        int32 GetReceivedImages();

        void CreateImageProcessTasks() override;
        void UpdateBuffer() override;
        void UpdateTasks() override;
        void UpdateLiveMode() override;

        MemPool<int16>* m_objMemPool16bit;
        int16* m_ptrImg16bit;
    };

    //////////////////////////////////////////////////
    /// LambdaDataReceiver24BitMode OPERATION_MODE_24
    //////////////////////////////////////////////////
    /**
     * @biref data receiver for 24bit mode
     */
    class LambdaDataReceiver24BitMode : public LambdaDataReceiver
    {
    public:
        LambdaDataReceiver24BitMode();
        ~LambdaDataReceiver24BitMode();

        bool Init(double dShutter,Enum_readout_mode& mode, LambdaConfigReader& objConfig) override;
        void Start() override;
        void Stop() override;

        int32 GetReceivedDecodedImages() override;
        int32 GetQueueDepth() override;
        int32* GetDecodedImageInt(int32& lFrameNo, int16& shErrCode) override;

    private:
        void InitMemoryPool();
        int32 GetReceivedImages();

        void CreateImageProcessTasks() override;
        void UpdateBuffer() override;
        void UpdateTasks() override;
        void UpdateLiveMode() override;

        MemPool<int32>* m_objMemPool32bit;
        int32* m_ptrImg32bit;
    };

    //////////////////////////////////////////////////
    /// LambdaDataReceiver2X12BitMode OPERATION_MODE_2X12
    //////////////////////////////////////////////////
}
