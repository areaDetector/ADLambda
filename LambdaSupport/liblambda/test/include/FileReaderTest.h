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

#ifndef __FILE_READER_TEST_H__
#define __FILE_READER_TEST_H__

#include <iostream>
#include <memory>
#include <cppunit/extensions/HelperMacros.h>

#include <Globals.h>

namespace DetUnitTestNS
{

    using namespace DetCommonNS;
    
    class CheckEquality;
    
    class FileReaderTest : public CppUnit::TestFixture
    {
        //add to test suite
        CPPUNIT_TEST_SUITE(FileReaderTest);
        CPPUNIT_TEST(TestEquality);
        CPPUNIT_TEST(TestOpen);
        CPPUNIT_TEST_SUITE_END();
        
      public:
        FileReaderTest();

        void setUp();
        void tearDown();
        
        void TestEquality();
        void TestOpen();
        //void TestDataContent();
        

      private:
        std::unique_ptr<CheckEquality> m_uPtrCheckEqual;
        std::unique_ptr<FileOperation> m_uPtrFileOp;
        // std::unique_ptr<LambdaConfigReader> m_uPtrLambdaConfig;
        std::string m_strFilePath;
    };
}

#endif
