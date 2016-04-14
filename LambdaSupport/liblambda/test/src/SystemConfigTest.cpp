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
#include "SystemConfigTest.h"
#include "LambdaInterface.h"
//#include "LambdaSysImpl.h"
// #include "LambdaFactory.h"
// #include "Lambda12bit1link.h"
// #include "Lambda24bit1link.h"
// #include "Lambda12bit2links.h"
// #include "Lambda24bit2links.h"

namespace DetUnitTestNS
{
    CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SystemConfigTest,"AllTest");

    
    void SystemConfigTest::setUp()
    {
        m_strFilePath=std::string("./filetest");

        // m_uPtrFac = std::unique_ptr<LambdaFactory>(new LambdaDetFactory());
        //m_uPtrInterface = m_uPtrFac->CreateProduct(strFilePath,false,"");
        //  m_uPtrInterface = std::unique_ptr<LambdaSysImpl>(new LambdaSysImpl());
    }
    
    
    void SystemConfigTest::tearDown()
    {
        // m_uPtrCheckEqual.reset();
        // m_uPtrFileOp.reset();

        // m_uPtrFac.reset();
        //m_uPtrInterface.reset();
        
    }
  
    void SystemConfigTest::TestInit()
    {
        
    }
    
}
