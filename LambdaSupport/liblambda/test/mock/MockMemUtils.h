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

#ifndef __LAMBDA_MEM_UTILS_H__
#define __LAMBDA_MEM_UTILS_H__

#include "Globals.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    /**
     * @brief Memory pool
     * This class implements a memory pool. This pool provides all the memory operations for the whole library
     */
    template<class T>
        class MemPool : public boost::noncopyable
    {
      public:
        /**
         * @brief constructor 
         */
        MemPool()
        {
        }

        /**
         * @brief constructor
         * @param _nSize size of the pool
         * @param _nElementSize size of each image
         */
      MemPool(int _nSize,int _nElementSize)
          :m_nSize(_nSize)
            ,m_nElemSize(_nElementSize)
            ,m_lStoredImageNumbers(0)
            ,m_lLastArrived(0)
            ,m_lStartPos(0)
            ,m_lEndPos(0)
            ,m_bstMtx()
            ,m_lCurrentWorkingFrame(0)
            ,m_lTotalReceivedFrames(0)
            ,m_bStartAcq(true)
            ,m_bFrameStart(true)
            ,m_lStartFrameNo(-1)
            ,m_lLastUnfinishedFrame(-1)
            ,m_lNewFrameNo(-1)
            ,m_lFirstFrameNumber(-1)
            ,m_bFirstFrameSet(false)
            ,m_lTotalReceivedPackets(0)
        {	
         
        }

        /**
         * @brief destructor 
         */
        ~MemPool()
        {
          
        }

        void Reset()
        {  
          
        }

        /**
         * @brief allocate memory
         */
        void AllocateMem()
        {
          
        }

        /**
         * @brief increase current memory
         * @param nIncreasingSize the size that will be increased
         */
        void IncreaseMem(int nIncreasingSize)
        {
        }

        void AddTaskFrame(int nID, int nFrameNo)
        {
           
        }
        
        void PrintLog()
        {
           
        }
        
        /**
         * @brief get already stored images in the buffer
         * @return stored images
         */
        long GetStoredImageNumbers() const
        {
          
        }

        long GetFirstFrameNo() const
        {
          
        }
	
        void SetFirstFrameNo(const long lFrameNo)
        {
           
        }

        long GetTotalReceivedFrames() const
        {
          
        }
 
        /**
         * @brief get latest arrived image no
         * @return lastest image no
         */
        long GetLastArrivedImageNo() const
        {
           
        }

        /**
         * @brief check if images are ready for reading
         * @param nFrameNumbers how many frames needs to be read in one time
         * @return true: ready for reading; false not ready yet
         */
        bool IsImageReadyForReading(const int nFrameNumbers) const
        {
           
        }

        bool IsImageFinished()
        {
           
        }

        void UpdateFrame(bool bFullFinished)
        {
          
        }
        /**
         * @brief get start position of the buffer
         */
        long GetStartPosOfBuffer() const
        {
          
        }

        long GetTotalReceivedPackets() const
        {
    
        }
        

        long GetFrameNoByIndex(const long lIdx) const
        {
          
        }

        int GetElementSize() const
        {
         
        }

        /**
         * @brief tell if buffer is full
         * @return true: full; false: not full
         */
        bool IsFull() const
        {
          
        }

        void SetRequestedPacketNumber(const int nPacketNumbers)
        {
           
        }

         /**
         * @brief get two images from buffer
         * @param objImg1 image buffer
         * @param lImgNo1 img No
         * @param shErrCode1 error code
         * @param objImg2 image buffer
         * @param lImgNo2
         * @param shErrCode2
         */
        bool Get2Image(T* objImg1,long& lImgNo1,short& shErrCode1,T* objImg2,long& lImgNo2,short& shErrCode2)
        {
          
            
        }
        
        /**
         * @brief get one image from buffer
         * @param objImg image buffer
         * @param lImgNo img No
         * @param shErrCode error code
         */
        bool GetImage(T* objImg,long& lImgNo,short& shErrCode)
        {
          
        }


        bool SetPacket(T* objPacket,const int nPos,const int nLength,const long lImgNo,const short shErrCode,const int nTaskID,int shPacketNo)
        {   
           
        }

        /**
         * @brief store one image into buffer
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         */
        bool SetImage(T* objImg,const long lImgNo,const short shErrCode)
        {
         
        }

        /**
         * @brief store one image into buffer
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         * @param bFixedPosQueue the position for each frame is fixed through the frame no
         */
        bool SetImage(T* objImg,const long lImgNo,const short shErrCode,bool bFixedPosQueue)
        {
      
        }
        
      private:
        /**
         * @brief get relative buffer position by frame no
         * @param lFrameNo frame no
         * @return relative position
         */
        long GetPosByFrameNo(const long lFrameNo) const
        {
          
        }

        /**
         * @brief get start postision frame
         * @return current frame no
         */
        long GetCurrentFrameNo() const
        { 
        }

        /**
         * @brief get next consecutive frame no
         * @return next frame no
         */
        long GetNextFrameNo() const
        {
        }

        /**
         * @brief relase memory
         */
        void ReleaseMem()
        {   
        }


      private:
        int m_nSize;
        int m_nElemSize;
        long m_lStoredImageNumbers;
        long m_lLastArrived;
        //oldest image position in buffer
        long m_lStartPos;
        //newest image position in buffer
        long m_lEndPos;
        long m_lCurrentWorkingFrame;
        int m_nRequestedPacketsNumber;
        long m_lTotalReceivedFrames;
        long m_lTotalReceivedPackets;
        long m_lStartFrameNo;
        long m_lLastUnfinishedFrame;
        long m_lNewFrameNo;
        long m_lFirstFrameNumber;
        bool m_bFirstFrameSet;
        
        vector<T*,boost::fast_pool_allocator<T*> > m_vImgBuff;
        vector<long> m_vFrameNumber;
        vector<short> m_vErrorCode;   
        mutable boost::mutex m_bstMtx;
        vector<long> m_vTaskMonitor;
        vector<int> m_vAcquiredPacketNumbers;
        vector<string> vGlobalDebugInfo;
        bool m_bStartAcq;
        bool m_bFrameStart;
        
    };///end of class MemPool
}///end of namespace DetCommonNS

#endif
