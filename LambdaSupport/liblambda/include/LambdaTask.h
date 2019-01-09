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
#include <math.h>

///namespace DetCommonNS
namespace DetLambdaNS
{
    using namespace FSDetCoreNS;
    
    class LambdaSysImpl;
    class LambdaModule;
    /**
     * @brief LambdaTask class
     */
    class LambdaTask : public Task
    {
      public:
        /**
         * @brief constructor
         */
        LambdaTask();

        /**
         * @brief constructor
         * @param _strTaskName task name
         * @param _Epriority task priority
         * @param _nID task ID
         * @param _objSys LambdaSysImpl object
         * @param _objNetInt network object
         * @param _objMemPoolRaw raw buffer
         * @param _objMemPoolDecoded12 12bit decoded image buffer
         * @param _objMemPoolDecoded24 24bit decoded image buffer
         * @param _bstMtx boost mutex for acquiring images
         * @param _vCurrentChip current used chips
         * @param _vNIndex index for distortion correction
         * @param _vNNominator nominator
         */

        LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                   LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                   MemPool<char>* _objMemPoolRaw,MemPool<int16>* _objMemPoolDecoded12,
                   boost::mutex* _bstMtx,vector<int16> _vCurrentChip,vector<int32> _vNIndex,
                   vector<int32> _vNNominator);
        
        LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                   LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                   MemPool<char>* _objMemPoolRaw,MemPool<int32>* _objMemPoolDecoded24,
                   boost::mutex* _bstMtx,vector<int16> _vCurrentChip,vector<int32> _vNIndex,
                   vector<int32> _vNNominator);

        LambdaTask(string _strTaskName, Enum_priority _Epriority, int32 _nID,
                   LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,
                   MemPool<char>* _objMemPoolRaw,MemPool<char>* _objMemPoolCompressed,
                   boost::mutex* _bstMtx,vector<int16> _vCurrentChip,
                   vector<int32> _vNIndex,vector<int32> _vNNominator,int32 _nDistortedImageSize);
        
        /**
         * @brief desctructor
         */
        ~LambdaTask();

        void SetCompressedBuffer(MemPool<char>* objMemPoolCompressed);
        
        /**
         * @brief do task action
         */
        void DoTaskAction();
        /**
         * @brief monitor listener, check if image is finished
         */
        void DoMonitorListener();

        /**
         * @brief do acquisition with multi link setting
         */
        void DoAcquisitionWithMultiLink();
        
        /**
         * @brief do acquisition task
         */
        void DoAcquisition();

        /**
         * @brief do acquisition task with TCP link for tests (requires special handling of data)
         */
        void DoAcquisitionTCP();
	
        /**
         * @brief decode image task
         */
        void DoDecodeImage();

      private:
        LambdaSysImpl* m_objSys;
        NetworkInterface* m_objNetInterface;
        MemPool<char>* m_objMemPoolRaw;
        MemPool<char>* m_objMemPoolCompressed;
        MemPool<int16>* m_objMemPoolDecodedShort;
        MemPool<int32>* m_objMemPoolDecodedInt;
        boost::mutex* m_boostMtx;
        vector<int16> m_vCurrentChip;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;
        int32 m_nRawImageSize;
        int32 m_nDecodedImageSize;
    };///end of class LambdaTask
}
