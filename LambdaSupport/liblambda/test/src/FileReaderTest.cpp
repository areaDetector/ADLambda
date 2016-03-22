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

#include "FilesOperation.h"
#include "CheckEquality.h"
#include "FileReaderTest.h"

//#include "LambdaConfigReader.h"

namespace DetUnitTestNS
{
    CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FileReaderTest,"AllTest");

    FileReaderTest::FileReaderTest()
    {}
    
    void FileReaderTest::setUp()
    {
        m_uPtrCheckEqual = std::unique_ptr<CheckEquality>(new CheckEquality());
        m_uPtrFileOp = std::unique_ptr<FileOperation>(new FileReader());
        m_strFilePath=std::string("./filetest");
    }
    
    
    void FileReaderTest::tearDown()
    {
        m_uPtrCheckEqual.reset();
        m_uPtrFileOp.reset();
    }
    
    void FileReaderTest::TestEquality()
    {
        //   m_uPtrCheckEqual->CheckEqualityInt(1,2);    
    }

    void FileReaderTest::TestOpen()
    {
        string strFileTrue = m_strFilePath+"/SystemConfig.txt";
        string strFileFalse = m_strFilePath+"/NoThisFile.txt";
        
        m_uPtrFileOp->SetFilePath(strFileTrue);
        CPPUNIT_ASSERT(m_uPtrFileOp->OpenFile(false)==true);
        m_uPtrFileOp->CloseFile();

        // m_uPtrFileOp->SetFilePath(strFileFalse);
        // CPPUNIT_ASSERT(m_uPtrFileOp->OpenFile(false)==false);
        // m_uPtrFileOp->CloseFile();
    }
    
}
