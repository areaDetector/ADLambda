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
#include "MemUtils.h"

namespace FSDetCoreNS
{
    template <class T>
    MemPool<T>::MemPool(szt _nSize, szt _nElementSize)
        : m_nSize(_nSize),
          m_nElemSize(_nElementSize),
          m_lStoredImageNumbers(0),
          m_lLastArrived(0),
          m_lStartPos(0),
          m_lEndPos(0),
          m_lCurrentWorkingFrame(0),
          m_nRequestedPacketsNumber(0),
          m_lTotalReceivedFrames(0),
          m_lTotalReceivedPackets(0),
          m_lStartFrameNo(-1),
          m_lLastUnfinishedFrame(-1),
          m_lNewFrameNo(-1),
          m_lFirstFrameNumber(-1),
          m_bFirstFrameSet(false),
          m_nSafetyMargin(32),
          m_bstMtx(),
          m_bStartAcq(true),
          m_bFrameStart(true)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_nAllowedImages = m_nSize - m_nSafetyMargin;
        
        m_vFrameNumber.resize(m_nSize, int32min);
        m_vErrorCode.resize(m_nSize, int16min);
        m_vDataSize.resize(m_nSize, 0);
        m_vAcquiredPacketNumbers.resize(m_nSize, 0);
        AllocateMem();
    }
    
    template <class T>
    MemPool<T>:: ~MemPool()
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        LOG_TRACE(__FUNCTION__);
        ReleaseMem();
        m_vFrameNumber.clear();
        m_vErrorCode.clear();
        m_vAcquiredPacketNumbers.clear();
        m_vTaskMonitor.clear();
        vGlobalDebugInfo.clear();
    }

    template <class T>
    void MemPool<T>::Reset()
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        m_lLastArrived = 0;
        m_lStartPos = 0;
        m_lEndPos = 0;
        m_lStoredImageNumbers = 0;
        m_lTotalReceivedFrames = 0;
        m_lTotalReceivedPackets = 0;
        m_nRequestedPacketsNumber = 0;
        m_lCurrentWorkingFrame = 0;
        std::fill(m_vFrameNumber.begin(), m_vFrameNumber.end(), int32min);
        std::fill(m_vErrorCode.begin(), m_vErrorCode.end(), int16min);
        std::fill(m_vDataSize.begin(), m_vDataSize.end(), 0);

        std::fill(m_vAcquiredPacketNumbers.begin(), m_vAcquiredPacketNumbers.end(), 0);
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
    
    template <class T>
    void MemPool<T>::AllocateMem()
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        // allocate the memory
        for(szt i = 0; i < m_nSize; i++) {
            T* objImg = new T[m_nElemSize];
            m_vImgBuff.push_back(objImg);
        }
    }
    
    template <class T>
    void MemPool<T>::IncreaseMem(int32 nIncreasingSize)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        for(int i = 0; i < nIncreasingSize; i++) {
            T* objImg = new T[m_nElemSize];
            m_vImgBuff.push_back(objImg);
        }
        m_nSize += nIncreasingSize;
        m_nAllowedImages = m_nSize - m_nSafetyMargin;
        m_vFrameNumber.resize(m_nSize, int32min);
        m_vErrorCode.resize(m_nSize, int16min);
    }
    
    template <class T>
    void MemPool<T>::AddTaskFrame(int32 nID, int32 nFrameNo)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        m_vTaskMonitor.push_back(-1);
    }
    
    template <class T>
    void MemPool<T>::PrintLog()
    {
        for(szt i = 0; i < vGlobalDebugInfo.size(); i++)
            cout << vGlobalDebugInfo[i];
        vGlobalDebugInfo.clear();
    }
    
    template <class T>   
    int32 MemPool<T>::GetStoredImageNumbers() const
    {
        int32 lVal = m_lStoredImageNumbers;
        return lVal;
    }
    
    template <class T>   
    int32 MemPool<T>::GetFirstFrameNo() const
    {
        int32 lVal = m_lFirstFrameNumber;
        return lVal;
    }
    
    template <class T>
    void MemPool<T>::SetFirstFrameNo(int32 lFrameNo)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        m_lFirstFrameNumber = lFrameNo;
        int32 lPos = GetPosByFrameNo(m_lFirstFrameNumber);
        m_lStartPos = lPos;
        m_lEndPos = lPos;
        lock.unlock();
    }
    
    template <class T>
    int32 MemPool<T>::GetTotalReceivedFrames() const
    {
        int32 lVal = m_lTotalReceivedFrames;
        return lVal;
    }
    
    template <class T>
    int32 MemPool<T>::GetLastArrivedImageNo() const
    {
        int32 lVal = m_lLastArrived;
        return lVal;
    }
    
    template <class T>
    bool MemPool<T>::IsImageReadyForReading(int32 nFrameNumbers) const
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        bool bVal = false;
        if(nFrameNumbers == 1)
            bVal = (GetCurrentFrameNo() != int32min);
        else if(nFrameNumbers == 2) {
            int32 lFrame1 = GetCurrentFrameNo();
            int32 lFrame2 = GetNextFrameNo();
            if((lFrame1 < lFrame2) && (lFrame2 - lFrame1 == 1)
               && (lFrame2 % 2 == 0)
               && (lFrame1 != int32min)
               && (lFrame2 != int32min))
                bVal = true;
            else
                bVal = false;
        }
        lock.unlock();
        return bVal;
    }
    
    template <class T>
    void MemPool<T>::IsImageFinished()
    {
        // frame finished, all packets received
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        if(m_vAcquiredPacketNumbers[m_lEndPos] == m_nRequestedPacketsNumber) {
            lock.unlock();
            UpdateFrame(true);
        }
        else    // check current working on frame
        {
            int32 lVal = *min_element(m_vTaskMonitor.begin(), m_vTaskMonitor.end());
            // all bigger than current lImgNo by at least 1
            if(lVal > (m_lLastUnfinishedFrame + 1) && lVal != -1)   
            {
                lock.unlock();
                UpdateFrame(false);
            }
        }
    }
    
    template <class T>
    void MemPool<T>::UpdateFrame(bool bFullFinished)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        if(m_vAcquiredPacketNumbers[m_lEndPos] != 133)
        {
            string strTmp2 = "Raw image received incomplete: taskid:||"
                + to_string(m_lLastUnfinishedFrame)
                + "||" + to_string(m_lTotalReceivedFrames)
                + "||" + to_string(m_vAcquiredPacketNumbers[m_lEndPos])
                + "||";
                
            for(szt j = 0; j < m_vTaskMonitor.size(); j++)
            {
                strTmp2 += to_string(j);
                strTmp2 += ":";
                strTmp2 += to_string(m_vTaskMonitor[j]);
                strTmp2 += "||";
            }
            strTmp2 += "\n";
            vGlobalDebugInfo.push_back(strTmp2);
        }
            
        m_vAcquiredPacketNumbers[m_lEndPos] = 0;
        m_lLastArrived = m_lLastUnfinishedFrame;
        m_vFrameNumber[m_lEndPos] = m_lLastUnfinishedFrame;
        if(bFullFinished)
            m_vErrorCode[m_lEndPos] = 0;
        else
            m_vErrorCode[m_lEndPos] = -1;
        m_lLastUnfinishedFrame += 1;
        m_lStoredImageNumbers++;
        m_lTotalReceivedFrames++;
        m_lEndPos = (m_lEndPos + 1) % (m_nSize);
    }
    
    template <class T>
    int32 MemPool<T>::GetStartPosOfBuffer() const
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        int32 lVal = m_lStartPos;
        lock.unlock();
        return lVal;
    }
    
    template <class T>
    int32 MemPool<T>::GetTotalReceivedPackets() const
    {
        return m_lTotalReceivedPackets;
    }
    
    template <class T>
    int32 MemPool<T>::GetFrameNoByIndex(const int32 lIdx) const
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        int32 lVal = m_vFrameNumber[lIdx];
        lock.unlock();
        return lVal;
    }
    
    template <class T>
    szt MemPool<T>::GetElementSize() const
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        return m_nElemSize;
    }
    
    template <class T>
    bool MemPool<T>::IsFull() const
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        bool bVal = (static_cast<szt>(m_lStoredImageNumbers) >= m_nAllowedImages);
        lock.unlock();
        return bVal;
    }
    
    template <class T>
    void MemPool<T>::SetRequestedPacketNumber(int32 nPacketNumbers)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        m_nRequestedPacketsNumber = nPacketNumbers;
        lock.unlock();
    }
    
    template <class T>
    bool MemPool<T>::Get2Image(T*& objImg1, int32& lImgNo1, int16& shErrCode1,
                            T*& objImg2, int32& lImgNo2, int16& shErrCode2)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        bool bVal = false;

        int32 lFrame1 = GetCurrentFrameNo();
        int32 lFrame2 = GetNextFrameNo();
        if((lFrame1 < lFrame2)
           && (lFrame2 - lFrame1 == 1)
           && (lFrame1 != int32min)
           && (lFrame2 != int32min))
            bVal = true;
        else
            bVal = false;

        if(!bVal) {
            LOG_INFOS("No image is in buffer!!!");
            lock.unlock();
            return false;
        } else {

            int nTempPos = m_lStartPos;

            // get first image
            // This will be pointer to image array
            objImg1 = m_vImgBuff[m_lStartPos];

            lImgNo1 = m_vFrameNumber[m_lStartPos];
            m_vFrameNumber[m_lStartPos] = int32min;

            shErrCode1 = m_vErrorCode[m_lStartPos];
            m_vErrorCode[m_lStartPos] = int16min;

            m_lStoredImageNumbers--;
            m_lStartPos = (m_lStartPos + 1) % (m_nSize);
            // Don't copy, just return pointer
            // std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg1);

            // get second image
            objImg2 = m_vImgBuff[m_lStartPos];

            lImgNo2 = m_vFrameNumber[m_lStartPos];
            m_vFrameNumber[m_lStartPos] = int32min;

            shErrCode2 = m_vErrorCode[m_lStartPos];
            m_vErrorCode[m_lStartPos] = int16min;

            m_lStoredImageNumbers--;
            m_lStartPos = (m_lStartPos + 1) % (m_nSize);
            // Don't copy, just return pointer
            // std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg2);

            // check image image is really valid--
            // hot fix,should think about this, seems that this is a thread sync problem
            if(lImgNo1 == int32min || lImgNo2 == int32min) {
                LOG_STREAM(__FUNCTION__, ERROR, "Frame numbers are not correct.");
                // LOG_INFOS("Frame numbers are not correct.");
                m_lStartPos = nTempPos;
                m_lStoredImageNumbers += 2;

                return false;
            }

            return true;
        }
    }
    
    template <class T>
    bool MemPool<T>::GetImage(T*& objImg, int32& lImgNo, int16& shErrCode, int32& nDataSize)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);

        bool bVal = false;
        bVal = (GetCurrentFrameNo() != int32min);
        if(!bVal) {
            LOG_INFOS("No image is in buffer!!!");
            lock.unlock();
            return false;
        } else {
            int32 nTempPos = m_lStartPos;
            objImg = m_vImgBuff[m_lStartPos];

            lImgNo = m_vFrameNumber[m_lStartPos];
            m_vFrameNumber[m_lStartPos] = int32min;

            shErrCode = m_vErrorCode[m_lStartPos];
            m_vErrorCode[m_lStartPos] = int16min;

            nDataSize = m_vDataSize[m_lStartPos];
            m_vDataSize[m_lStartPos] = 0;

            m_lStoredImageNumbers--;
            m_lStartPos = (m_lStartPos + 1) % (m_nSize);

            // Don't copy, just return pointer
            // std::copy(objImgTmp,objImgTmp+m_nElemSize,objImg);

            // check image image is really valid--hot fix,
            // should think about this, seems that this is a thread sync problem
            if(lImgNo == int32min) {
                LOG_STREAM(__FUNCTION__, ERROR, "Frame number are not correct.");
                m_lStartPos = nTempPos;
                m_lStoredImageNumbers += 1;

                return false;
            }
            return true;
        }
    }
    
    template <class T>
    bool MemPool<T>::SetPacket(T* objPacket, szt nPos, szt nLength,
                               int32 lImgNo, int16 shErrCode,int32 nTaskID, uint16 shPacketNo)
    {
        int32 lPos = GetPosByFrameNo(lImgNo);

        copy(objPacket + UDP_EXTRA_BYTES, objPacket + nLength, m_vImgBuff[lPos] + nPos);

        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        
        if(m_bFrameStart) {
            m_lStartPos = lPos;
            m_lEndPos = lPos;
            m_lLastUnfinishedFrame = lImgNo;
            m_lFirstFrameNumber = lImgNo;
            m_bFrameStart = false;
        }

        m_vAcquiredPacketNumbers[lPos]++;
        m_lTotalReceivedPackets++;
        m_vTaskMonitor[nTaskID] = lImgNo;
        return true;
    }
    
    template <class T>
    bool MemPool<T>::SetImage(T* objImg, int32 lImgNo, int16 shErrCode)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);

        if(static_cast<szt>(m_lStoredImageNumbers) >= m_nAllowedImages) {

            LOG_INFOS("Buffer is already full!!!");
            lock.unlock();
            return false;
        }
        m_lLastArrived = lImgNo;
        m_lStoredImageNumbers++;
        m_lTotalReceivedFrames++;

        m_vFrameNumber[m_lEndPos] = lImgNo;
        m_vErrorCode[m_lEndPos] = shErrCode;
        copy(objImg, objImg + m_nElemSize, m_vImgBuff[m_lEndPos]);
        m_lEndPos = (m_lEndPos + 1) % (m_nSize);

        return true;
    }
    
    template <class T>
    bool MemPool<T>::SetImage(T* objImg, int32 lImgNo, int16 shErrCode,
                           bool bFixedPosQueue, szt nDataSize)
    {
        boost::unique_lock<boost::mutex> lock(m_bstMtx);
        // Check if image falls into "safety margin" region - if so, don't allow insertion.
        int32 lPos = GetPosByFrameNo(lImgNo);
        int32 lSafetyTest = lPos - m_lStartPos;
        if(lSafetyTest < 0)
            lSafetyTest += m_nSize;    // This avoids odd behaviour of modulus with negatives

        if(static_cast<szt>(lSafetyTest) >= m_nAllowedImages) {
            return false;
        }

        bool bVal = (m_vFrameNumber[lPos] == int32min);

        if(bVal) {
            // Insertion operation should be done with mutex
            // unlocked to prevent blocking in this case
            // Rely on correct image numbering here
            // Only update queue info etc after insertion,
            // to prevent invalid images from causing problems.
            lock.unlock();

            if(nDataSize == 0)
                copy(objImg, objImg + m_nElemSize, m_vImgBuff[lPos]);
            else {

                copy(objImg, objImg + nDataSize, m_vImgBuff[lPos]);
                m_vDataSize[lPos] = nDataSize;
            }

            lock.lock();

            m_lLastArrived = lImgNo;    // Not guaranteed to be correct - depends on ordering
            m_lStoredImageNumbers++;

            m_vFrameNumber[lPos] = lImgNo;
            m_vErrorCode[lPos] = shErrCode;

            return true;
        }

        return false;
    }
    
    template <class T>
    int32 MemPool<T>::GetPosByFrameNo(int32 lFrameNo) const
    {
        int32 lVal = (lFrameNo - 1) % m_nSize;
        return lVal;
    }
    
    template <class T>
    int32 MemPool<T>::GetCurrentFrameNo() const
    {
        int32 lVal = m_vFrameNumber[m_lStartPos];
        return lVal;
    }

    template <class T>
    int32 MemPool<T>::GetNextFrameNo() const
    {
        int32 lVal = m_vFrameNumber[((m_lStartPos + 1) % m_nSize)];
        return lVal;
    }

    template <class T>
    void MemPool<T>::ReleaseMem()
    {
        // clear the memory
        // Loop through image buffer and deallocate
        for(szt i = 0; i < m_vImgBuff.size(); i++)
        {
            delete[] m_vImgBuff[i];
        }
    }
}
