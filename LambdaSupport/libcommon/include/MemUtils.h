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
            ,m_nSafetyMargin(32)
        {	
            LOG_TRACE(__FUNCTION__);
            m_nAllowedImages = m_nSize - m_nSafetyMargin;
            m_vFrameNumber.resize(m_nSize,LONG_MIN);
            m_vErrorCode.resize(m_nSize,SHRT_MIN);
            m_vAcquiredPacketNumbers.resize(m_nSize,0);
            AllocateMem();
        }

        /**
         * @brief destructor 
         */
        ~MemPool()
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            LOG_TRACE(__FUNCTION__);
            ReleaseMem();  
            m_vFrameNumber.clear();
            m_vErrorCode.clear();
            m_vAcquiredPacketNumbers.clear();
            m_vTaskMonitor.clear();
            vGlobalDebugInfo.clear();
            lock.unlock();
        }

        void Reset()
        {  
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            m_lLastArrived = 0;
            m_lStartPos = 0;
            m_lEndPos = 0;
            m_lStoredImageNumbers = 0;
            m_lTotalReceivedFrames = 0;
            m_lTotalReceivedPackets = 0;
            m_lCurrentWorkingFrame = 0;
            std::fill(m_vFrameNumber.begin(),m_vFrameNumber.end(),LONG_MIN);
            std::fill(m_vErrorCode.begin(),m_vErrorCode.end(),SHRT_MIN);
            std::fill(m_vAcquiredPacketNumbers.begin(),m_vAcquiredPacketNumbers.end(),0);
            m_vTaskMonitor.clear();
            m_bStartAcq = true;
            m_bFrameStart = true;
            m_lStartFrameNo = -1;
            m_lLastUnfinishedFrame = -1;
            m_lNewFrameNo = -1;
            m_lFirstFrameNumber = -1;
            m_bFirstFrameSet = false;
            
            vGlobalDebugInfo.clear();
            lock.unlock();
        }

        /**
         * @brief allocate memory
         */
        void AllocateMem()
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            //allocate the memory
            for(int i=0;i<m_nSize;i++)
            {
                T* objImg = new T[m_nElemSize];	
                m_vImgBuff.push_back(objImg);
            }
            lock.unlock();
        }

        /**
         * @brief increase current memory
         * @param nIncreasingSize the size that will be increased
         */
        void IncreaseMem(int nIncreasingSize)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            for(int i=0;i<nIncreasingSize;i++)
            {
                T* objImg = new T[m_nElemSize];
                m_vImgBuff.push_back(objImg);
            }
            m_nSize+=nIncreasingSize;
            m_nAllowedImages = m_nSize - m_nSafetyMargin;
            m_vFrameNumber.resize(m_nSize,LONG_MIN);
            m_vErrorCode.resize(m_nSize,SHRT_MIN);
            lock.unlock();
        }

        void AddTaskFrame(int nID, int nFrameNo)
        {
            // LOG_TRACE(__FUNCTION__);
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            m_vTaskMonitor.push_back(-1);
            lock.unlock();
        }
        
        void PrintLog()
        {
            for(int i=0;i<vGlobalDebugInfo.size();i++)
                cout<<vGlobalDebugInfo[i];
            vGlobalDebugInfo.clear();
        }
        
        /**
         * @brief get already stored images in the buffer
         * @return stored images
         */
        long GetStoredImageNumbers() const
        {
            // LOG_TRACE(__FUNCTION__);
            long lVal = m_lStoredImageNumbers;
            return lVal;
        }

        long GetFirstFrameNo() const
        {
            long lVal = m_lFirstFrameNumber;
            return lVal;
        }
	
        void SetFirstFrameNo(const long lFrameNo)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            m_lFirstFrameNumber = lFrameNo;
            long lPos = GetPosByFrameNo(m_lFirstFrameNumber);
            m_lStartPos = lPos;
            m_lEndPos = lPos;
            lock.unlock();
        }

        long GetTotalReceivedFrames() const
        {
            long lVal = m_lTotalReceivedFrames;
            return lVal;
        }
 
        /**
         * @brief get latest arrived image no
         * @return lastest image no
         */
        long GetLastArrivedImageNo() const
        {
            //LOG_TRACE(__FUNCTION__);
            long lVal = m_lLastArrived;
            return lVal;
        }

        /**
         * @brief check if images are ready for reading
         * @param nFrameNumbers how many frames needs to be read in one time
         * @return true: ready for reading; false not ready yet
         */
        bool IsImageReadyForReading(const int nFrameNumbers) const
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            bool bVal = false;
            if(nFrameNumbers == 1)
                bVal = (GetCurrentFrameNo() != LONG_MIN);
            else if(nFrameNumbers == 2)
            {
                long lFrame1 = GetCurrentFrameNo();
                long lFrame2 = GetNextFrameNo();
                if((lFrame1<lFrame2)&&(lFrame2-lFrame1 == 1) && (lFrame2%2 == 0) 
                   && (lFrame1 != LONG_MIN) && (lFrame2 != LONG_MIN))
                    bVal = true;
                else 
                    bVal = false;

            }
            lock.unlock();
            return bVal;
        }

        bool IsImageFinished()
        {
            //frame finished, all packets received
	    boost::unique_lock<boost::mutex> lock(m_bstMtx);
            if(m_vAcquiredPacketNumbers[m_lEndPos] == m_nRequestedPacketsNumber)
	    {
	        lock.unlock();
                UpdateFrame(true);
	    }
            else //check current working on frame
            {
                long lVal = *min_element(m_vTaskMonitor.begin(),m_vTaskMonitor.end());
                if(lVal>(m_lLastUnfinishedFrame+1) && lVal!=-1)//all bigger than current lImgNo by at least 1
		{
		    lock.unlock();
                    UpdateFrame(false);
		}
            }
        }

        void UpdateFrame(bool bFullFinished)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            if(m_vAcquiredPacketNumbers[m_lEndPos]!=133)
            {
                string strTmp2 = "Raw image received incomplete: taskid:||"+to_string(static_cast<long long>(m_lLastUnfinishedFrame))+"||"+to_string(static_cast<long long>(m_lTotalReceivedFrames))+"||"
                    +to_string(static_cast<long long>(m_vAcquiredPacketNumbers[m_lEndPos]))+"||";
                for(int j=0;j<m_vTaskMonitor.size();j++)
                {
                    strTmp2+=to_string(static_cast<long long>(j));
                    strTmp2+=":";
                    strTmp2+=to_string(static_cast<long long>(m_vTaskMonitor[j]));
                    strTmp2+="||";
                }
                strTmp2+="\n";
                vGlobalDebugInfo.push_back(strTmp2);
            }
            m_lLastArrived = m_lLastUnfinishedFrame;
            m_vFrameNumber[m_lEndPos] = m_lLastUnfinishedFrame;
            if(bFullFinished)
                m_vErrorCode[m_lEndPos] = 0;
            else
                m_vErrorCode[m_lEndPos] = -1;
            m_lLastUnfinishedFrame +=1;
            m_lStoredImageNumbers++;
            m_lTotalReceivedFrames++;
            m_lEndPos = (m_lEndPos+1)%(m_nSize); 
	  
            lock.unlock();
        }
        /**
         * @brief get start position of the buffer
         */
        long GetStartPosOfBuffer() const
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            //LOG_TRACE(__FUNCTION__);
            long lVal = m_lStartPos;
            lock.unlock();
            return lVal;
        }

        long GetTotalReceivedPackets() const
        {
            return m_lTotalReceivedPackets;
        }
        

        long GetFrameNoByIndex(const long lIdx) const
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            long lVal = m_vFrameNumber[lIdx];
            lock.unlock();
            return lVal;
        }

        int GetElementSize() const
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            //LOG_TRACE(__FUNCTION__);
            int nVal = m_nElemSize;
            lock.unlock();
            return nVal;
        }

        /**
         * @brief tell if buffer is full
         * @return true: full; false: not full
         */
        bool IsFull() const
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            bool bVal = (m_lStoredImageNumbers >= m_nAllowedImages);
            lock.unlock();
            return bVal;
        }

        void SetRequestedPacketNumber(const int nPacketNumbers)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            m_nRequestedPacketsNumber = nPacketNumbers;
            lock.unlock();
        }

         /**
         * @brief get two images from buffer
         * @param objImg1 image buffer - this is a POINTER to the data that will remain valid for "m_nSafetyMargin" inserts
         * @param lImgNo1 img No
         * @param shErrCode1 error code
         * @param objImg2 image buffer - this is a POINTER to the data that will remain valid for "m_nSafetyMargin" inserts
         * @param lImgNo2
         * @param shErrCode2
         */
        bool Get2Image(T*& objImg1,long& lImgNo1,short& shErrCode1,T*& objImg2,long& lImgNo2,short& shErrCode2)
        {
            /* if(!IsImageReadyForReading(2)) */
            /* { */
            /*     boost::unique_lock<boost::mutex> lock(m_bstMtx); */
            /*     LOG_INFOS("No image is in buffer!!!"); */
            /*     lock.unlock(); */
            /*     return false;    */
            
            /* } */

            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            bool bVal = false;
            
            long lFrame1 = GetCurrentFrameNo();
            long lFrame2 = GetNextFrameNo();
            if((lFrame1<lFrame2)&&(lFrame2-lFrame1 == 1) 
               && (lFrame1 != LONG_MIN) && (lFrame2 != LONG_MIN))
                bVal = true;
            else 
                bVal = false;

            if(!bVal)
            {
                LOG_INFOS("No image is in buffer!!!");
                lock.unlock();
                return false;
            }
            else
            {
                
                int nTempPos = m_lStartPos;
                
                // get first image
                // This will be pointer to image array
                objImg1 =  m_vImgBuff[m_lStartPos];

                lImgNo1 = m_vFrameNumber[m_lStartPos];
                m_vFrameNumber[m_lStartPos] = LONG_MIN;

                shErrCode1 = m_vErrorCode[m_lStartPos];
                m_vErrorCode[m_lStartPos] = SHRT_MIN;

                m_lStoredImageNumbers--;
                m_lStartPos = (m_lStartPos+1)%(m_nSize);
                // Don't copy, just return pointer
                //std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg1);

                //get second image
                objImg2 =  m_vImgBuff[m_lStartPos];

                lImgNo2 = m_vFrameNumber[m_lStartPos];
                m_vFrameNumber[m_lStartPos] = LONG_MIN;

                shErrCode2 = m_vErrorCode[m_lStartPos];
                m_vErrorCode[m_lStartPos] = SHRT_MIN;

                m_lStoredImageNumbers--;
                m_lStartPos = (m_lStartPos+1)%(m_nSize);
                // Don't copy, just return pointer
                // std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg2);
                
                //check image image is really valid--hot fix,should think about this, seems that this is a thread sync problem
                if(lImgNo1== LONG_MIN || lImgNo2 == LONG_MIN)
                {
                    LOG_STREAM(__FUNCTION__,ERROR,"Frame numbers are not correct.");
                    //LOG_INFOS("Frame numbers are not correct.");
                    m_lStartPos = nTempPos;
                    m_lStoredImageNumbers+=2;
                    
                    lock.unlock();
                    return false;
                }

                lock.unlock();
                return true;
            }
            
        }
        
        /**
         * @brief get one image from buffer
         * @param objImg image buffer
         * @param lImgNo img No
         * @param shErrCode error code
         */
        bool GetImage(T*& objImg,long& lImgNo,short& shErrCode)
        {
            /* if(!IsImageReadyForReading(1)) */
            /* { */
            /*     boost::unique_lock<boost::mutex> lock(m_bstMtx); */
            /*     LOG_INFOS("No image is in buffer!!!"); */
            /*     lock.unlock(); */
            /*     return false;    */
            
            /* } */
            /* else */
            /* { */
            boost::unique_lock<boost::mutex> lock(m_bstMtx);

            bool bVal = false;
            bVal = (GetCurrentFrameNo() != LONG_MIN);
            if(!bVal)
            {
                    
                LOG_INFOS("No image is in buffer!!!");
                lock.unlock();
                return false;
            }   
            else
            {
                    
                
                int nTempPos = m_lStartPos;
                objImg = m_vImgBuff[m_lStartPos];

                lImgNo = m_vFrameNumber[m_lStartPos];
                m_vFrameNumber[m_lStartPos] = LONG_MIN;

                shErrCode = m_vErrorCode[m_lStartPos];
                m_vErrorCode[m_lStartPos] = SHRT_MIN;

                m_lStoredImageNumbers--;
                m_lStartPos = (m_lStartPos+1)%(m_nSize);

                // Don't copy, just return pointer
                // std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg);
                
                //check image image is really valid--hot fix,should think about this, seems that this is a thread sync problem
                if(lImgNo == LONG_MIN)
                {
                    LOG_STREAM(__FUNCTION__,ERROR,"Frame number are not correct.");
                    //LOG_INFOS("Frame numbers are not correct.");
                    m_lStartPos = nTempPos;
                    m_lStoredImageNumbers+=1;
                    
                    lock.unlock();
                    return false;
                }
                lock.unlock();
                return true;
            }
        }


        bool SetPacket(T* objPacket,const int nPos,const int nLength,const long lImgNo,const short shErrCode,const int nTaskID,int shPacketNo)
        {   
            long lPos = GetPosByFrameNo(lImgNo);
            
            copy(objPacket+UDP_EXTRA_BYTES,objPacket+nLength,m_vImgBuff[lPos]+nPos);

            // Lock mutex only when updating variables
            // lock.lock();
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
            
            if(m_bFrameStart)
            {
                m_lStartPos = lPos;
                m_lEndPos = lPos;
                m_lLastUnfinishedFrame = lImgNo;
                m_lFirstFrameNumber = lImgNo;
                m_bFrameStart = false;
            }
            
            m_vAcquiredPacketNumbers[lPos]++;
            m_lTotalReceivedPackets++;
            m_vTaskMonitor[nTaskID] = lImgNo;
            lock.unlock();
            return true;
        }

        /**
         * @brief store one image into buffer
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         */
        bool SetImage(T* objImg,const long lImgNo,const short shErrCode)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);

            if(m_lStoredImageNumbers >= m_nAllowedImages)
            {

                LOG_INFOS("Buffer is already full!!!");
                lock.unlock();
                return false;
            }
            m_lLastArrived = lImgNo;
            m_lStoredImageNumbers++;
            m_lTotalReceivedFrames++;
	    
            m_vFrameNumber[m_lEndPos] = lImgNo;
            m_vErrorCode[m_lEndPos] = shErrCode;         
            copy(objImg,objImg+m_nElemSize,m_vImgBuff[m_lEndPos]);	
            m_lEndPos = (m_lEndPos+1)%(m_nSize);

            lock.unlock();
            return true;
        }

        /**
         * @brief store one image into buffer - position fixed by frame no
         * @param objImg image data
         * @param lImgNo image No.
         * @param shErrCode error code
         * @param bFixedPosQueue the position for each frame is fixed through the frame no
         */
        bool SetImage(T* objImg,const long lImgNo,const short shErrCode,bool bFixedPosQueue)
        {
            boost::unique_lock<boost::mutex> lock(m_bstMtx);
	    // Check if image falls into "safety margin" region - if so, don't allow insertion.
	    long lPos = GetPosByFrameNo(lImgNo);
	    long lSafetyTest = lPos-m_lStartPos;
	    if(lSafetyTest < 0) lSafetyTest+=m_nSize; // This avoids odd behaviour of modulus with negatives

            if(lSafetyTest >= m_nAllowedImages)
            {
                lock.unlock();
                return false;
            }

            bool bVal = (m_vFrameNumber[lPos] == LONG_MIN);

            if(bVal)
            {				    
	        // Insertion operation should be done with mutex unlocked to prevent blocking in this case
	        // Rely on correct image numbering here
	        // Only update queue info etc after insertion, to prevent invalid images from causing problems.
	        lock.unlock();

		copy(objImg,objImg+m_nElemSize,m_vImgBuff[lPos]);
	        
		lock.lock();

	        m_lLastArrived = lImgNo; // Not guaranteed to be correct - depends on ordering
                m_lStoredImageNumbers++;

                m_vFrameNumber[lPos] = lImgNo;
                m_vErrorCode[lPos] = shErrCode;
                lock.unlock(); 
                	    
                return true;
            }
            else
            {
                lock.unlock();
                return false;
            }	  	    
        }
        
      private:
        /**
         * @brief get relative buffer position by frame no
         * @param lFrameNo frame no
         * @return relative position
         */
        long GetPosByFrameNo(const long lFrameNo) const
        {
            //LOG_TRACE(__FUNCTION__);
            long lVal = (lFrameNo-1)%m_nSize;
            return lVal;
        }

        /**
         * @brief get start postision frame
         * @return current frame no
         */
        long GetCurrentFrameNo() const
        {
            //LOG_TRACE(__FUNCTION__);
            long lVal = m_vFrameNumber[m_lStartPos];
            return lVal;
        }

        /**
         * @brief get next consecutive frame no
         * @return next frame no
         */
        long GetNextFrameNo() const
        {
            //LOG_TRACE(__FUNCTION__);
            long lVal = m_vFrameNumber[((m_lStartPos+1)%m_nSize)];
            return lVal;
        }

        /**
         * @brief relase memory
         */
        void ReleaseMem()
        {
            //LOG_TRACE(__FUNCTION__);
            //clear the memory
            boost::singleton_pool<boost::fast_pool_allocator_tag,sizeof(T*)>::release_memory();
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
        int m_nSafetyMargin; // When checking if buffer is full, report as full when number of unoccupied
            // images equals safety margin - ensures pointers remain valid for long enough during readout.
        int m_nAllowedImages;
        
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
