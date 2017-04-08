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

#include "ThreadUtils.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    //////////////////////////////////////////////////
    ///Task
    //////////////////////////////////////////////////
    Task::Task()
    {
        LOG_TRACE(__FUNCTION__);
    }

    Task::~Task()
    {
        LOG_TRACE(__FUNCTION__);
    }
  
    string Task::GetTaskName() const
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_strTaskName;
    }

    Enum_priority Task::GetPriority() const
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_enumPriority;
    }

    void Task::SetTargetPriority(Enum_priority * _targetPriority)
    {
        LOG_TRACE(__FUNCTION__);

        m_enumTargetPriority = _targetPriority;
    }

    int Task::GetID() const
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_nID;
    }

    void Task::DoTaskAction() 
    {
        LOG_TRACE(__FUNCTION__);
    }
    
    //////////////////////////////////////////////////
    ///ThreadPool
    //////////////////////////////////////////////////
    ThreadPool::ThreadPool()
    {
        LOG_TRACE(__FUNCTION__);
    }

    ThreadPool::ThreadPool(int _nThreadNumber)
        :m_nThreadNumber(_nThreadNumber)
        ,m_nAvailableThreads(_nThreadNumber)
        ,m_bRunning(true)
        ,m_boostMtx()
        ,m_boostCond()
        ,m_enumCurrentPriority(NORMAL)
    {
        LOG_TRACE(__FUNCTION__);
        
        LOG_INFOS("ThreadPool is initializing...");

        for(int i=0;i<m_nThreadNumber;i++)
            m_boostThGroup.create_thread(boost::bind(&ThreadPool::Run,this));      
    }
        
    ThreadPool::~ThreadPool()
    {
        LOG_TRACE(__FUNCTION__);

        {
            boost::unique_lock<boost::mutex> lock(m_boostMtx);
            m_bRunning = false;
            m_boostCond.notify_all();    
        }

        try
        {
            m_boostThGroup.join_all();
        }
        catch(...)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"Exiting thread is wrong");
        }
        
        LOG_INFOS("Thread pool is closed. All threads exit");
    }
    
    void ThreadPool::AddTask(void* objTask)
    {
        LOG_TRACE(__FUNCTION__);	
      
        boost::unique_lock<boost::mutex> lock(m_boostMtx);
        Task* objTsk = (Task*)objTask;
        
        if(m_nAvailableThreads == 0)
        {
            LOG_INFOS("No thread is available for running the task!!!");
            return;
        }

        m_nAvailableThreads--;
        m_qTaskList.push(objTsk);
        m_boostCond.notify_one();
    }

    int ThreadPool::GetAvailableThreads()
    {
        boost::unique_lock<boost::mutex> lock(m_boostMtx);
        return m_nAvailableThreads;
    }

    void ThreadPool::SetPriorityLevel(Enum_priority _newPriority)
    {
        boost::unique_lock<boost::mutex> lock(m_boostMtx);
        m_enumCurrentPriority = _newPriority;
    }

    
    void ThreadPool::Run()
    {
        // LOG_TRACE(__FUNCTION__);
        while(m_bRunning)
        {
            boost::unique_lock<boost::mutex> lock(m_boostMtx);
            while(m_qTaskList.empty() && m_bRunning)
            {
                LOG_INFOS("Thread is waiting");
                m_boostCond.wait(lock);
            }
	
            if(!m_bRunning)
                break;

            {
                Task* objTask = m_qTaskList.front();
                m_qTaskList.pop();
                lock.unlock();	
	      
                try
                {
                    objTask->SetTargetPriority(&m_enumCurrentPriority); // Thread pool can now control priority
                    objTask->DoTaskAction();
                    delete objTask;
                }
                catch(...)
                {
                    LOG_STREAM(__FUNCTION__,ERROR,"Running task is wrong");
                }		
            }
            
            lock.lock();
            m_nAvailableThreads++;

        }
    }    
}///end of namespace DetCommonNS


