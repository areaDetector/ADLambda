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
    //class FSDetCoreNS::Task;

    class ImageDecoder;

    template<class T>
    class DistortionCorrector;


    typedef unique_ptr<ImageDecoder> uptr_Decoder;
    typedef unique_ptr<DistortionCorrector<int16>> uptr_DC16;
    typedef unique_ptr<DistortionCorrector<int32>> uptr_DC32;
    typedef unique_ptr<CompressionContext> uptr_Compressor;

    //////////////////////////////////////////////////
    /// LambdaRecvTask
    //////////////////////////////////////////////////
    class LambdaRecvTask : public Task
    {
    public:
        virtual ~LambdaRecvTask(){}
        virtual void SetRequestedImages(int32 /* unImgNo */){};
    };

    //////////////////////////////////////////////////
    /// LambdaTaskMultiLinkUDPRecv
    //////////////////////////////////////////////////
    /**
     * @brief LambdaTask class for multi link udp receiver
     */
    class LambdaTaskMultiLinkUDPRecv : public LambdaRecvTask
    {
    public:
        LambdaTaskMultiLinkUDPRecv(string strTaskName, Enum_priority Epriority, int32 nID,
                                   NetworkInterface* objNetInt,
                                   MemPool<char>* objMemPoolRaw);

        ~LambdaTaskMultiLinkUDPRecv(){}

        /**
         * @brief set requested images
         */
        void SetRequestedImages(int32 nImgNo) override;

        /**
         * @brief do task action
         */
        void DoTaskAction();

    private:
        /**
         * @brief monitor listener, check if image is finished
         */
        void DoMonitorListener();

        /**
         * @brief do acquisition with multi link setting
         */
        void DoAcquisitionWithMultiLink();

        NetworkInterface* m_objNetInterface;
        MemPool<char>* m_objMemPoolRaw;
        int32 m_nRawImageSize;
        int32 m_nRequestedImageNo;
    };

    //////////////////////////////////////////////////
    /// LambdaTaskSingleLinkUDPRecv
    //////////////////////////////////////////////////
    /**
     * @brief LambdaTask class for single link udp receiver
     */
    class LambdaTaskSingleLinkUDPRecv : public LambdaRecvTask
    {
    public:
        LambdaTaskSingleLinkUDPRecv(string strTaskName, Enum_priority Epriority, int32 nID,
                                    NetworkInterface* objNetInt,
                                    MemPool<char>* objMemPoolRaw);

        ~LambdaTaskSingleLinkUDPRecv(){};

        /**
         * @brief set requested images
         */
        void SetRequestedImages(int32 nImgNo) override;

        /**
         * @brief do task action
         */
        void DoTaskAction();

    private:
        /**
         * @brief do acquisition with single link setting
         */
        void DoAcquisitionWithSingleLink();

        NetworkInterface* m_objNetInterface;
        MemPool<char>* m_objMemPoolRaw;
        int32 m_nRawImageSize;
        int32 m_nRequestedImageNo;
    };

    //////////////////////////////////////////////////
    /// LambdaTaskSingleLinkTCPRecv
    //////////////////////////////////////////////////
    /**
     * @brief LambdaTask class for single link udp receiver
     */
    class LambdaTaskSingleLinkTCPRecv : public LambdaRecvTask
    {
    public:
        LambdaTaskSingleLinkTCPRecv(string strTaskName, Enum_priority Epriority, int32 nID,
                                    NetworkInterface* objNetInt,
                                    MemPool<char>* objMemPoolRaw);

        ~LambdaTaskSingleLinkTCPRecv(){};

        /**
         * @brief set requested images
         */
        void SetRequestedImages(int32 nImgNo) override;

        /**
         * @brief do task action
         */
        void DoTaskAction();

    private:
        /**
         * @brief do acquisition with single link setting
         */
        void DoAcquisitionWithSingleLink();

        NetworkInterface* m_objNetInterface;
        MemPool<char>* m_objMemPoolRaw;
        int32 m_nRawImageSize;
        int32 m_nRequestedImageNo;
    };

    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage
    //////////////////////////////////////////////////
    class LambdaTaskDecodeImage : public Task
    {
    public:
        virtual ~LambdaTaskDecodeImage();

        /**
         * @brief enable distortion correction
         * @param vNIndex index of the pixel
         * @param vNNominator nominator of the pixel
         */
        virtual void EnableDC(vector<int32>& vNIndex, vector<int32>& vNNominator) = 0;

        /**
         * @brief disable distortion correction
         */
        virtual void DisableDC() = 0;

        /**
         * @brief set live mode
         * @param nFrameRate frame rate
         * @param pLiveImg live image
         */
        virtual void SetLiveMode(int32 nFrameRate, int32& nFrameNo, int32* pLiveImg) = 0;

        /**
         * @brief set buffer for images
         * @param objMemPoolCompressed compressed buffer
         */
        virtual void SetBuffer(MemPool<char>* objMemPoolCompressed){}

        /**
         * @brief set buffer for images
         * @param objMemPoolDecoded12 12bit image buffer
         */
        virtual void SetBuffer(MemPool<int16>* objMemPoolDecoded12){}

        /**
         * @brief set buffer for images
         * @param objMemPoolDecoded24 24bit image buffer
         */
        virtual void SetBuffer(MemPool<int32>* objMemPoolDecoded24){}

        void EnableCompression(int16 nType,int16 nCompressionLevel = 2);
        void EnableCompression(int32 nChunkIn,int32 nChunkOut,int32 nPostCode);
        void DisableCompression();

    protected:
        LambdaTaskDecodeImage();
        virtual bool DoDecodeImage() = 0;
        virtual void DoDistortionCorrection() = 0;
        virtual void SetFirstFrameNo() = 0;
        virtual void DoCompression() = 0;
        virtual void WriteData() = 0;
        virtual void UpdateLiveImage() = 0;

        void DoTaskAction() override;

        uptr_Compressor m_pCompressor;
        int32 m_nCompressionLevel;
        bool m_fDCEnabled,m_fCompressionEnabled;
		vector<uchar> m_vDstData;
    };

    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage12
    //////////////////////////////////////////////////
    class LambdaTaskDecodeImage12 : public LambdaTaskDecodeImage
    {
    public:
        LambdaTaskDecodeImage12(string strTaskName, Enum_priority Epriority, int32 nID,
                                vector<int16> vCurrentChip,
                                MemPool<char>* objMemPoolRaw,
                                int32 nImageSizeAfterDC);

        ~LambdaTaskDecodeImage12();

        void EnableDC(vector<int32>& vNIndex, vector<int32>& vNNominator) override;
        void DisableDC() override;
        void SetLiveMode(int32 nFrameRate, int32& nFrameNo, int32* pLiveImg) override;
        void SetBuffer(MemPool<char>* objMemPoolCompressed) override;
        void SetBuffer(MemPool<int16>* objMemPoolDecoded12) override;

    private:
        bool DoDecodeImage() override;
        void DoDistortionCorrection() override;
        void DoCompression() override;
        void SetFirstFrameNo() override;
        void WriteData() override;
        void UpdateLiveImage() override;

        void WriteData(char* pCompressedData, int32 nFrameNo, int16 shErrorCode, int32 nDataSize);
        void WriteData(int16* p12bitImg, int32 nFrameNo, int16 shErrorCode);

        vector<int16> m_vCurrentChip;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;

        int32 m_nRawImageSize;
        int32 m_nDecodedImageSize;
        int32 m_nImageSizeBeforeDC;
        int32 m_nSubImages;

        MemPool<char>* m_objMemPoolRaw;
        MemPool<char>* m_objMemPoolCompressed;
        MemPool<int16>* m_objMemPoolDecoded12;

        uptr_Decoder m_pDecoder;
        uptr_DC16 m_pDC16;

        int32* m_pLiveImage;
        int32* m_nLiveFrameNo;
        int32 m_nFrameRate;

        int16* m_p12bitDecodedImg;
        char* m_pCompressedData;

        vector<uchar> m_vSrcData;
        int32 m_nFrameNo;
        int16 m_shErrCode;
        int32 m_nDataLength;
    };

    //////////////////////////////////////////////////
    /// LambdaTaskDecodeImage24
    //////////////////////////////////////////////////
    class LambdaTaskDecodeImage24 : public LambdaTaskDecodeImage
    {
    public:
        LambdaTaskDecodeImage24(string strTaskName, Enum_priority Epriority, int32 nID,
                                vector<int16> vCurrentChip,
                                MemPool<char>* objMemPoolRaw,
                                int32 nImageSizeAfterDC);

        ~LambdaTaskDecodeImage24();

        void EnableDC(vector<int32>& vNIndex, vector<int32>& vNNominator) override;
        void DisableDC() override;
        void SetLiveMode(int32 nFrameRate, int32& nFrameNo, int32* pLiveImg) override;
        void SetBuffer(MemPool<char>* objMemPoolCompressed) override;
        void SetBuffer(MemPool<int32>* objMemPoolDecoded24) override;

    private:
        bool DoDecodeImage() override;
        void DoDistortionCorrection() override;
        void DoCompression() override;
        void SetFirstFrameNo() override;
        void WriteData() override;
        void UpdateLiveImage() override;

        void WriteData(char* pCompressedData, int32 nFrameNo, int16 shErrorCode, int32 nDataSize);
        void WriteData(int32* p24bitImg, int32 nFrameNo, int16 shErrorCode);

        vector<int16> m_vCurrentChip;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;

        int32 m_nRawImageSize;
        int32 m_nDecodedImageSize;
        int32 m_nImageSizeBeforeDC;
        int32 m_nSubImages;

        MemPool<char>* m_objMemPoolRaw;
        MemPool<char>* m_objMemPoolCompressed;
        MemPool<int32>* m_objMemPoolDecoded24;

        uptr_Decoder m_pDecoder;
        uptr_DC32 m_pDC32;

        int32* m_pLiveImage;
        int32* m_nLiveFrameNo;
        int32 m_nFrameRate;

        int32* m_p24bitDecodedImg;
        int32* m_p24bitFinalImg;

        char* m_pCompressedData;

        vector<uchar> m_vSrcData;
        int32 m_nFrameNo;
        int16 m_shErrCode;
        int32 m_nDataLength;
    };
}
