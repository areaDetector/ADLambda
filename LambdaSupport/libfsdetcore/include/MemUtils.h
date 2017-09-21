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

#include "Globals.h"

namespace FSDetCoreNS
{
    /**
     * @brief Memory pool
     * This class implements a memory pool.
     * This pool provides all the memory operations for the whole library
     */
    template <class T>
    class MemPool : public boost::noncopyable
    {
    public:
        /**
         * @brief constructor
         * @param _nSize size of the pool
         * @param _nElementSize size of each image
         */
        MemPool(szt _nSize, szt _nElementSize);

        /**
         * @brief destructor
         */
        ~MemPool();
        
        void Reset();
        
        /**
         * @brief allocate memory
         */
        void AllocateMem();
        

        /**
         * @brief increase current memory
         * @param nIncreasingSize the size that will be increased
         */
        void IncreaseMem(int32 nIncreasingSize);
        void AddTaskFrame(int32 nID, int32 nFrameNo);

        void PrintLog();
        

        /**
         * @brief get already stored images in the buffer
         * @return stored images
         */
        int32 GetStoredImageNumbers() const;
        

        int32 GetFirstFrameNo() const;
        

        void SetFirstFrameNo(int32 lFrameNo);
        

        int32 GetTotalReceivedFrames() const;
        

        /**
         * @brief get latest arrived image no
         * @return lastest image no
         */
        int32 GetLastArrivedImageNo() const;
        
        /**
         * @brief check if images are ready for reading
         * @param nFrameNumbers how many frames needs to be read in one time
         * @return true: ready for reading; false not ready yet
         */
        bool IsImageReadyForReading(int32 nFrameNumbers) const;
        
        void IsImageFinished();
        
        void UpdateFrame(bool bFullFinished);
        
        /**
         * @brief get start position of the buffer
         */
        int32 GetStartPosOfBuffer() const;
        

        int32 GetTotalReceivedPackets() const;
        
        
        int32 GetFrameNoByIndex(const int32 lIdx) const;
        

        szt GetElementSize() const;
        

        /**
         * @brief tell if buffer is full
         * @return true: full; false: not full
         */
        bool IsFull() const;
        

        void SetRequestedPacketNumber(int32 nPacketNumbers);
        

        /**
         * @brief get two images from buffer
         * @param objImg1 image buffer -
         *        this is a POINTER to the data that will remain valid for "m_nSafetyMargin" inserts
         * @param lImgNo1 img No
         * @param shErrCode1 error code
         * @param objImg2 image buffer -
         *        this is a POINTER to the data that will remain valid for "m_nSafetyMargin" inserts
         * @param lImgNo2
         * @param shErrCode2
         */
        bool Get2Image(T*& objImg1, int32& lImgNo1, int16& shErrCode1,
                       T*& objImg2, int32& lImgNo2, int16& shErrCode2);
        
        /**
         * @brief get one image from buffer
         * @param objImg image buffer
         * @param lImgNo img No
         * @param shErrCode error code
         * @param nDataSize data size
         */
        bool GetImage(T*& objImg, int32& lImgNo, int16& shErrCode, int32& nDataSize);
        

        bool SetPacket(T* objPacket, szt nPos, szt nLength,
                       int32 lImgNo, int16 shErrCode,int32 nTaskID, uint16 shPacketNo);
        

        /**
         * @brief store one image into buffer
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         */
        bool SetImage(T* objImg, int32 lImgNo, int16 shErrCode);
        
        /**
         * @brief store one image into buffer - position fixed by frame no
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         * @param bFixedPosQueue the position for each frame is fixed through the frame no
         * @param nDataSize data size
         */
        bool SetImage(T* objImg, int32 lImgNo, int16 shErrCode,
                      bool bFixedPosQueue, szt nDataSize = 0);
        

    private:
        /**
         * @brief get relative buffer position by frame no
         * @param lFrameNo frame no
         * @return relative position
         */
        int32 GetPosByFrameNo(int32 lFrameNo) const;

        /**
         * @brief get start postision frame
         * @return current frame no
         */
        int32 GetCurrentFrameNo() const;
        
        /**
         * @brief get next consecutive frame no
         * @return next frame no
         */
        int32 GetNextFrameNo() const;
        

        /**
         * @brief relase memory
         */
        void ReleaseMem();

    private:
        szt m_nSize;
        szt m_nElemSize;
        int32 m_lStoredImageNumbers;
        int32 m_lLastArrived;
        // oldest image position in buffer
        int32 m_lStartPos;
        // newest image position in buffer
        int32 m_lEndPos;
        int32 m_lCurrentWorkingFrame;
        int m_nRequestedPacketsNumber;
        int32 m_lTotalReceivedFrames;
        int32 m_lTotalReceivedPackets;
        int32 m_lStartFrameNo;
        int32 m_lLastUnfinishedFrame;
        int32 m_lNewFrameNo;
        int32 m_lFirstFrameNumber;
        bool m_bFirstFrameSet;
        szt m_nSafetyMargin;    // When checking if buffer is full,
                                // report as full when number of unoccupied
                                // images equals safety margin -
                                // ensures pointers remain valid for long enough during readout.
        szt m_nAllowedImages;

        vector<T*> m_vImgBuff;
        vector<int32> m_vFrameNumber;
        vector<int16> m_vErrorCode;
        vector<int32> m_vDataSize;
        mutable boost::mutex m_bstMtx;
        vector<int32> m_vTaskMonitor;
        vector<int32> m_vAcquiredPacketNumbers;
        vector<string> vGlobalDebugInfo;
        bool m_bStartAcq;
        bool m_bFrameStart;

    };
}

//implementation file
#include "MemUtils.tpp"
