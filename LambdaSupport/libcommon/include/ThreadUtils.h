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

#ifndef __THREAD_UTILS_H__
#define __THREAD_UTILS_H__

#include "Globals.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    /**
     * @brief task class 
     */
    class Task
    {
      public:
        /**
         * @brief constructor 
         */
        Task();
            
        /**
         * @brief destructor 
         */
        virtual ~Task();

        /**
         * @brief get task name
         * @return task name
         */
        virtual string GetTaskName() const;

        /**
         * @brief get priority
         * @return priority
         */
        virtual Enum_priority GetPriority() const;

        /**
         * @brief set target priority
         * @param pointer to priority variable to track, e.g. in thread pool.
         */
        virtual void SetTargetPriority(Enum_priority * _targetPriority);

        /**
         * @brief get task id
         * @return task id
         */
        virtual int GetID() const;

        virtual void DoTaskAction();
        
      protected:
        string m_strTaskName;
        Enum_priority m_enumPriority;
        Enum_priority * m_enumTargetPriority;
        int m_nID;
    };///end of class Task

    /**
     * @brief thread pool class
     * This class implements a thread pool which is used to manager all threads used in Lambda SDK
     */
    class ThreadPool
    {
      public:
        /**
         * @brief constructor 
         */
        ThreadPool();

        /**
         * @brief constructor
         * @param _nThreadNumber number of threads need to be created
         */
        ThreadPool(int _nThreadNumber);
            
        /**
         * @brief destructor 
         */
        ~ThreadPool();
        
        /**
         * @brief assign task to thread
         * @param objTask task
         */
        void AddTask(void* objTask);

        /**
         * @brief get avalable threads
         * @param available threads
         */
        int GetAvailableThreads();

        /**
         * @brief set prioritization of tasks
         * @param priority level - tasks lower than this may be throttled (implementation dependent)
         */
        void SetPriorityLevel(Enum_priority _newPriority);
        
      
      private:
        void Run();
                    
      private:
        int m_nThreadNumber;
        boost::thread_group m_boostThGroup;
        int m_nAvailableThreads;
        bool m_bRunning;
        queue< Task* > m_qTaskList;
        boost::mutex m_boostMtx;
        boost::condition_variable m_boostCond;
        Enum_priority m_enumCurrentPriority;
    };///end of class ThreadPool    
}///end of namespace DetCommonNS

#endif
