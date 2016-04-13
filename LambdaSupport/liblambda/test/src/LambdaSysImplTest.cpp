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
#include "LambdaSysImplTest.h"

///namespace DetUnitTestNS
namespace DetUnitTestNS
{
    LambdaSysImplTest::LambdaSysImplTest(vector<shared_ptr<NetworkInterface>> _objNetInt,shared_ptr<ThreadPool> _objThPool, shared_ptr<MemPool<int>> _objMemPool)
        :m_vspNetInt(_objNetInt),m_spThPool(_objThPool),m_spMemPoolInt(_objMemPool)
     {
         
     }

    LambdaSysImplTest::~LambdaSysImplTest()
    {
        
    }

    void LambdaSysImplTest::Init()
    {
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

        m_nDistortionCorrectionMethod = 1;
    }
    
    
    
}///end of namespace DetUnitTestNS
