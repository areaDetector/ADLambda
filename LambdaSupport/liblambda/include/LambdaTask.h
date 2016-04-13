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

#ifndef __LAMBDA_TASK_H__
#define __LAMBDA_TASK_H__

#include "LambdaGlobals.h"
#include "ThreadUtils.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    class LambdaSysImpl;
    class NetworkInterface;
    class LambdaModule;
    template<class T>
        class  MemPool;
   
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

        LambdaTask(string _strTaskName, Enum_priority _Epriority, int _nID,LambdaSysImpl* _objSys,NetworkInterface* _objNetInt, MemPool<char>* _objMemPoolRaw,MemPool<short>* _objMemPoolDecoded12,boost::mutex* _bstMtx,vector<short> _vCurrentChip,vector<int> _vNIndex,vector<int> _vNNominator);
        
        LambdaTask(string _strTaskName, Enum_priority _Epriority, int _nID, LambdaSysImpl* _objSys,NetworkInterface* _objNetInt,MemPool<char>* _objMemPoolRaw,MemPool<int>* _objMemPoolDecoded24,boost::mutex* _bstMtx,vector<short> _vCurrentChip,vector<int> _vNIndex,vector<int> _vNNominator);
        
        /**
         * @brief desctructor
         */
        ~LambdaTask();

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
         * @brief decode image task
         */
        void DoDecodeImage();

      private:
        LambdaSysImpl* m_objSys;
        NetworkInterface* m_objNetInterface;
        MemPool<char>* m_objMemPoolRaw;
        MemPool<short>* m_objMemPoolDecodedShort;
        MemPool<int>* m_objMemPoolDecodedInt;
        boost::mutex* m_boostMtx;
        int m_nRawImageSize;
        int m_nDecodedImageSize;
        vector<short> m_vCurrentChip;
        vector<int> m_vNIndex;
        vector<int> m_vNNominator;
    };///end of class LambdaTask
}///end of namespace DetCommonNS

#endif
