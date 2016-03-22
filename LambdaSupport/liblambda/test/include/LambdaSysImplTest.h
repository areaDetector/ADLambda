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

#ifndef __LAMBDA_SYSTEM_IMPL_TEST_H__
#define __LAMBDA_SYSTEM_IMPL_TEST_H__

#include "LambdaSysImpl.h"

///namespace DetUnitTestNS
namespace DetUnitTestNS
{

    using namespace DetCommonNS;
        
    class LambdaSysImplTest : public LambdaSysImpl
    {
      public:
        /**
         * @brief constructor
         */
        LambdaSysImplTest(vector<shared_ptr<NetworkInterface>> _objNetInt,shared_ptr<ThreadPool> _objThPool, shared_ptr<MemPool<int>> _objMemPool);
        
        /**
         * @brief destructor
         */
        ~LambdaSysImplTest();

        void Init();

      private:
        vector<shared_ptr<NetworkInterface>> m_vspNetInt;
        shared_ptr<ThreadPool> m_spThPool;
        shared_ptr<MemPool<int>> m_spMemPoolInt;
    };///end of class LambdaSysImplTest    
}///end of namespace DetUnitTestNS


#endif
