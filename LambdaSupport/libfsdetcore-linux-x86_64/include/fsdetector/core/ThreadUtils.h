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
        virtual void SetTargetPriority(Enum_priority* _targetPriority);

        /**
         * @brief get task id
         * @return task id
         */
        virtual int32 GetID() const;

        virtual void DoTaskAction();

        /**
         * @brief start task
         */
        virtual void Start();

        /**
         * @brief stop task
         */
        virtual void Stop();

        /**
         * @brief exit task
         */
        virtual void Exit();
        
      protected:
        string m_strTaskName;
        Enum_priority m_enumPriority;
        Enum_priority* m_enumTargetPriority;
        int32 m_nID;
        bool m_fStart,m_fExit;
        
        boost::condition_variable m_bstCond;
        boost::mutex m_bstSync;
    };

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
        ThreadPool(int32 _nThreadNumber);

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
        int32 GetAvailableThreads();

        /**
         * @brief set prioritization of tasks
         * @param priority level - tasks lower than this may be throttled (implementation dependent)
         */
        void SetPriorityLevel(Enum_priority _newPriority);

      private:
        void Run();

      private:
        int32 m_nThreadNumber;
        boost::thread_group m_boostThGroup;
        int32 m_nAvailableThreads;
        bool m_bRunning;
        queue<Task*> m_qTaskList;
        boost::mutex m_boostMtx;
        boost::condition_variable m_boostCond;
        Enum_priority m_enumCurrentPriority;
    };
}
