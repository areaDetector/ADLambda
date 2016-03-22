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

#include "LambdaTask.h"
#include "MemUtils.h"
#include "NetworkInterface.h"
#include "NetworkImplementation.h"
#include "LambdaModule.h"
#include "ImageDecoder.h"
#include "DistortionCorrector.h"
#include "LambdaSysImpl.h"

///namespace DetCommonNS
namespace DetCommonNS
{    
    //////////////////////////////////////////////////
    ///LambdaTask
    //////////////////////////////////////////////////
    LambdaTask::LambdaTask()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    LambdaTask::LambdaTask(string _strTaskName, Enum_priority _Epriority, int _nID, LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,MemPool<char>* _objMemPoolRaw,MemPool<short>* _objMemPoolDecoded12,boost::mutex* _bstMtx,vector<short> _vCurrentChip,vector<int> _vNIndex,vector<int> _vNNominator):
        m_objSys(_objSys)
        ,m_objNetInterface(_objNetInt)
        ,m_objMemPoolRaw(_objMemPoolRaw)
        ,m_objMemPoolDecodedShort(_objMemPoolDecoded12)
        ,m_boostMtx(_bstMtx)
        ,m_vCurrentChip(_vCurrentChip)
        ,m_vNIndex(_vNIndex)
        ,m_vNNominator(_vNNominator)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = _strTaskName;
        m_enumPriority = _Epriority;
        m_nID = _nID;
        m_enumTargetPriority = &m_enumPriority; // Default behaviour will ignore priority
        
        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
        m_nDecodedImageSize = m_objMemPoolDecodedShort->GetElementSize();
    }

     LambdaTask::LambdaTask(string _strTaskName, Enum_priority _Epriority, int _nID, LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,MemPool<char>* _objMemPoolRaw,MemPool<int>* _objMemPoolDecoded24,boost::mutex* _bstMtx,vector<short> _vCurrentChip,vector<int> _vNIndex,vector<int> _vNNominator):
        m_objSys(_objSys)
        ,m_objNetInterface(_objNetInt)
        ,m_objMemPoolRaw(_objMemPoolRaw)
        ,m_objMemPoolDecodedInt(_objMemPoolDecoded24)
        ,m_boostMtx(_bstMtx)
        ,m_vCurrentChip(_vCurrentChip)
        ,m_vNIndex(_vNIndex)
        ,m_vNNominator(_vNNominator)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_strTaskName = _strTaskName;
        m_enumPriority = _Epriority;
        m_nID = _nID;
        
        m_nRawImageSize = m_objMemPoolRaw->GetElementSize();
        m_nDecodedImageSize = m_objMemPoolDecodedInt->GetElementSize();
    }
    
    LambdaTask::~LambdaTask()
    {
        LOG_TRACE(__FUNCTION__);
    }

    void LambdaTask::DoTaskAction()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_strTaskName == "StartAcquisition")
            DoAcquisition();
        else if(m_strTaskName == "DecodeImage")       
            DoDecodeImage();
        else if(m_strTaskName == "MultiLink")
            DoAcquisitionWithMultiLink();
        else if(m_strTaskName == "MonitorTask")
            DoMonitorListener();
        m_objSys->GetState(); // When task exits, useful to check state.

    }
    
    void LambdaTask::DoMonitorListener()
    {
        LOG_TRACE(__FUNCTION__);

        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        long lRequestedImgNo = m_objSys->GetNImages();
        bool bIsStart = m_objSys->GetAcquisitionStart();
        if(m_objSys->GetOperationMode() == OPERATION_MODE_24)
            lRequestedImgNo*=2;
        lock.unlock();
        //cout<<"listner thread starts"<<m_nID<<endl;
        
        while(true)
        {   
            //while(m_objMemPoolRaw->GetTotalReceivedPackets()<(lRequestedImgNo*133*0.8))
            //    usleep(1000);
            usleep(200);
            
            if(m_objMemPoolRaw->GetTotalReceivedFrames()>=lRequestedImgNo || (!bIsStart))
                break;
	     
            m_objMemPoolRaw->IsImageFinished();
	     
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
        }
        //cout<<"listner thread exits"<<m_nID<<endl;
    }
    
    void LambdaTask::DoAcquisitionWithMultiLink()
    {    
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];    
        int nPacketSize = UDP_PACKET_SIZE_NORMAL;
        short shErrorCode = 0;
        long lFrameNo = 0;
        short shPacketSequenceNo = 0;
        int nPos = 0;
        m_objMemPoolRaw->AddTaskFrame(m_nID,-1);
        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        long lRequestedImgNo = m_objSys->GetNImages();
        bool bIsStart = m_objSys->GetAcquisitionStart();
        if(m_objSys->GetOperationMode() == OPERATION_MODE_24)
            lRequestedImgNo*=2;
        lock.unlock();
        int nCurrentFrame = -1;

        int nCount = 200;
        int nNoDataTimes = 0;
        

        //cout<<"listner thread starts"<<m_nID<<endl;
        while(true)
        {
            if(m_objMemPoolRaw->GetTotalReceivedFrames()>=lRequestedImgNo)
            {		
                break;
            }
            else if(!bIsStart)
            {
                //m_objNetInterface->ReceiveData(ptrchTmpImg,m_nRawImageSize);
                while(true)
                {
                    shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);
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
                break;
            }

            shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);
            
            if(shErrorCode!=-1)
            {
                lFrameNo = (unsigned char)ptrchPacket[5]
                    +(unsigned char)ptrchPacket[4]*256
                    +(unsigned char)ptrchPacket[3]*256*256;

                
                shPacketSequenceNo = (unsigned char)ptrchPacket[2];
                //if(shPacketSequenceNo == 1)
                //    nPos = 0;
                //else
                nPos = UDP_PACKET_SIZE_NORMAL+(UDP_PACKET_SIZE_NORMAL-UDP_EXTRA_BYTES)*(shPacketSequenceNo-2);
                m_objMemPoolRaw->SetPacket(ptrchPacket,nPos,nPacketSize,lFrameNo,shErrorCode,m_nID,shPacketSequenceNo);
            }
            // else
            // {
            //     if(m_objMemPoolRaw->GetTotalReceivedFrames()>=lRequestedImgNo || (!bIsStart))
            //         break;	    
            // }      
            
            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
        }
        
        lock.lock();
        m_objSys->SetAcquisitionStart(false);
        // m_objSys->SetState(ON);
        lock.unlock();
        //cout<<"listner thread exits"<<m_nID<<endl;
        delete ptrchPacket;
    }
  
    void LambdaTask::DoAcquisition()
    {
        LOG_TRACE(__FUNCTION__);
        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];    
        int nPacketSize = UDP_PACKET_SIZE_NORMAL;
        short shErrorCode = 0;
	
        char* ptrchTmpImg = new char[m_nRawImageSize];
        short nErrorCode = 0;
        long lFrameNo = 0;

        boost::unique_lock<boost::mutex> lock(*m_boostMtx);
        long lRequestedImgNo = m_objSys->GetNImages();
        if(m_objSys->GetOperationMode() == OPERATION_MODE_24)
            lRequestedImgNo*=2;
        bool bIsStart = m_objSys->GetAcquisitionStart();
        bool bIsFull = m_objMemPoolRaw->IsFull();
        lock.unlock();

        int nCount = 200;
        int nNoDataTimes = 0;
        
        while(true)
        {
            if(lFrameNo>=lRequestedImgNo)
            {		
                break;
            }
            else if(!bIsStart)
            {
                //m_objNetInterface->ReceiveData(ptrchTmpImg,m_nRawImageSize);
                while(true)
                {
                    shErrorCode = m_objNetInterface->ReceivePacket(ptrchPacket,nPacketSize);
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
                break;
            }

            //do image acquisition
            nErrorCode = m_objNetInterface->ReceiveData(ptrchTmpImg,m_nRawImageSize);
            lFrameNo++;
	    
            m_objMemPoolRaw->SetImage(ptrchTmpImg,lFrameNo,nErrorCode);	  
            LOG_INFOS(("Arrived Frame No is:"+to_string(static_cast<long long>(lFrameNo))));
            //cout<<"Arrived Frame No is:"<<to_string(static_cast<long long>(lFrameNo))<<endl;

            lock.lock();
            bIsStart = m_objSys->GetAcquisitionStart();
            lock.unlock();
        }///end of loop

        lock.lock();
        m_objSys->SetAcquisitionStart(false);
        //m_objSys->SetState(ON);
        lock.unlock();

        delete ptrchPacket;
        delete ptrchTmpImg;
        
        LOG_INFOS("Do acquisition thread exits");
    }

    void LambdaTask::DoDecodeImage()
    {
        LOG_TRACE(__FUNCTION__);
       
        char* ptrchTmpImg;
        char* ptrchTmpImg1;
	ptrchTmpImg = 0; // Null pointer initially
	ptrchTmpImg1 = 0;
        ImageDecoder* objDecoder = new ImageDecoder(m_vCurrentChip);
        ImageDecoder* objDecoder1 = new ImageDecoder(m_vCurrentChip);

        short* ptrshDecodedImg;
        short* ptrshDecodedImg1;
        int* ptrnDecodedImg = new int[m_nDecodedImageSize];
        int* ptrnFinishedImg;

        DistortionCorrector<short> *objDC = new DistortionCorrector<short>(m_vNIndex,m_vNNominator,(int)pow(2,12));
        DistortionCorrector<int> *objDC1 = new DistortionCorrector<int>(m_vNIndex,m_vNNominator,(int)pow(2,24));
        
        while(true)
        {
            usleep(20);

            boost::unique_lock<boost::mutex> lock(*m_boostMtx);
            bool bVal = m_objSys->SysExit();
            string strOperationMode =m_objSys->GetOperationMode();
            int nDistortionCorr = m_objSys->GetDistortionCorrecttionMethod();
            lock.unlock();

            if(bVal)
                break;

            if(*m_enumTargetPriority > m_enumPriority) // If priority lower, throttle back task
            {
                usleep(10000);
                continue;
            }

            //continuousReadWrite
            if(strOperationMode == OPERATION_MODE_12)
            {
                if((m_objMemPoolRaw->GetStoredImageNumbers() == 0) || (m_objMemPoolDecodedShort->IsFull()))
                {
                    usleep(50);
                    continue;
                }	  

		
                short shErrCode = 0;
                long lFrameNo = 0;    
                if(m_objMemPoolRaw->GetImage(ptrchTmpImg,lFrameNo,shErrCode))
		{
                    objDecoder->SetRawImage(ptrchTmpImg);
                    ptrshDecodedImg = objDecoder->RunDecodingImg();


                    if(nDistortionCorr == 1)
                    {
                        //distortion correction
                        short* pShImgOut = objDC->RunDistortCorrect(ptrshDecodedImg);
                        ptrshDecodedImg = pShImgOut;
                    }
            
                    if(m_objMemPoolDecodedShort->GetFirstFrameNo() == -1)
                    {
                        long lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                
                        //if lFristFrame is -1, means it is single link version
                        if(lFristFrame!=-1)
                            m_objMemPoolDecodedShort->SetFirstFrameNo(lFristFrame);
                    }    
                    //cout<<lFrameNo<<":is taken for decoding"<<endl;

                    while(true)
                    {
                        usleep(20);

                        lock.lock();
                        bool bIsAcq = m_objSys->GetAcquisitionStop();
                        lock.unlock();
                
                        if(bIsAcq)
                            break;
	    
                        if(m_objMemPoolDecodedShort->SetImage(ptrshDecodedImg,lFrameNo,shErrCode,true) == true)
			  {
                              break;
			  }
                    }
                }
                
            }
            else if(strOperationMode == OPERATION_MODE_24)
            {
                if((m_objMemPoolRaw->GetStoredImageNumbers() < 2) || (m_objMemPoolDecodedInt->IsFull()))
                {
                    usleep(50);
                    continue;
                }	  

                short shErrCode = 0;
                long lFrameNo = 0;
                short shErrCode1 = 0;
                long lFrameNo1 = 0;
                
                if(m_objMemPoolRaw->Get2Image(ptrchTmpImg,lFrameNo,shErrCode,ptrchTmpImg1,lFrameNo1,shErrCode1))
                {
                
                    objDecoder->SetRawImage(ptrchTmpImg);
                    ptrshDecodedImg = objDecoder->RunDecodingImg();

                    objDecoder1->SetRawImage(ptrchTmpImg1);
                    ptrshDecodedImg1 = objDecoder1->RunDecodingImg();

                    for(int i=0;i<m_nDecodedImageSize;i++)
                        ptrnDecodedImg[i] = ((int)ptrshDecodedImg[i])+(((int)ptrshDecodedImg1[i])*4096);
                    lFrameNo = lFrameNo1/2;
                    shErrCode = shErrCode<=shErrCode1?shErrCode:shErrCode1;
                    ptrnFinishedImg = ptrnDecodedImg;
                
                    if(nDistortionCorr == 1)
                    {
                        //distortion correction
                        int* pNImgOut = objDC1->RunDistortCorrect(ptrnDecodedImg);
                        ptrnFinishedImg = pNImgOut;
                    }
                
                    if(m_objMemPoolDecodedInt->GetFirstFrameNo() == -1)
                    {
                        long lFristFrame = m_objMemPoolRaw->GetFirstFrameNo();
                
                        //if lFristFrame is -1, means it is single link version
                        if(lFristFrame!=-1)
                            m_objMemPoolDecodedInt->SetFirstFrameNo((lFristFrame+1)/2);
                    }

                    while(true)
                    {
                        usleep(20);

                        lock.lock();
                        bool bIsAcq = m_objSys->GetAcquisitionStop();
                        lock.unlock();
                
                        if(bIsAcq)
                            break;
	    
                        if(m_objMemPoolDecodedInt->SetImage(ptrnFinishedImg,lFrameNo,shErrCode,true) == true)
                            break;
                    }
                }
                
            }
            
            
        }///end loop
        
        delete objDecoder;
        delete objDecoder1;
        delete ptrnDecodedImg;

        delete objDC;

        delete objDC1;

        LOG_INFOS("Do decoding thread exits");	
    }
}///end of namespace DetCommonNS
