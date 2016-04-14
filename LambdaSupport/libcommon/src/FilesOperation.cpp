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

namespace DetCommonNS
{
    //////////////////////////////////////////////////
    ///FileOperation
    //////////////////////////////////////////////////
    FileOperation::FileOperation()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    FileOperation::~FileOperation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    void FileOperation::SetFilePath(string strFilePath)
    {
        LOG_TRACE(__FUNCTION__);

        m_strFilePath = strFilePath;
    }
    vector<string> FileOperation::ReadDataFromFile()
    {
        LOG_TRACE(__FUNCTION__);
    }

    vector<int> FileOperation::ReadDataFromIntBinaryFile()
    {
        LOG_TRACE(__FUNCTION__);
    }
    
    vector<short> FileOperation::ReadDataFromBinaryFile()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    void FileOperation::WriteDataToFile()
    {
        LOG_TRACE(__FUNCTION__);
    }

    bool FileOperation::FileExists(const string strFileName)
    {
        LOG_TRACE(__FUNCTION__);
        
        struct stat stBuf;
        if(stat(strFileName.c_str(),&stBuf)!=-1)
            return true;
        return false;
    }
    
    //////////////////////////////////////////////////
    ///FileReader
    //////////////////////////////////////////////////
    FileReader::FileReader()
    {
        LOG_TRACE(__FUNCTION__);
    }
    
    FileReader::FileReader(string _strFilePath)
    {
        LOG_TRACE(__FUNCTION__);
        
        this->m_strFilePath = _strFilePath;
    }

    FileReader::~FileReader()
    {
        LOG_TRACE(__FUNCTION__);
        
        if(IsOpen())
            m_ifFileReader.close();
    }
        
    bool FileReader::OpenFile(bool bBinary)
    {
        LOG_TRACE(__FUNCTION__);

        try
        {
            if(!bBinary)
                m_ifFileReader.open(m_strFilePath,ios::in);
            else
                m_ifFileReader.open(m_strFilePath,ios::in|ios::binary);
                
            return true;
        }
        catch(...)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"File cannot open!");
            return false;
        }    
    }

    bool FileReader::IsOpen()
    {
        LOG_TRACE(__FUNCTION__);

        return m_ifFileReader.is_open();
    }

    void FileReader::CloseFile()
    {
        LOG_TRACE(__FUNCTION__);

        m_ifFileReader.close();    
    }

    vector<string> FileReader::ReadDataFromFile()
    {
        LOG_TRACE(__FUNCTION__);

        string strLine;
        if(!(this->m_vStrContent).empty())
            (this->m_vStrContent).clear();

        while(getline(m_ifFileReader,strLine))
            (this->m_vStrContent).push_back(strLine);
        return m_vStrContent;
    }

    vector<int> FileReader::ReadDataFromIntBinaryFile()
    {
        LOG_TRACE(__FUNCTION__);

        int intTemp;
        if(!m_vIntBinValues.empty())
            m_vIntBinValues.clear();
        
        if(IsOpen())
        {
            while(!m_ifFileReader.eof())
            {
                m_ifFileReader.read(reinterpret_cast<char*>(&intTemp),sizeof(int));
                if(!m_ifFileReader.eof())
                {
                    m_vIntBinValues.push_back(intTemp);
                }
            }
        }
        
        return m_vIntBinValues;
    }

    vector<short> FileReader::ReadDataFromBinaryFile()
    {
        LOG_TRACE(__FUNCTION__);
            
        short shTemp;
        if(!m_vShBinValues.empty())
            m_vShBinValues.clear();

        if(IsOpen())
        {
            while(!m_ifFileReader.eof())
            {
                m_ifFileReader.read(reinterpret_cast<char*>(&shTemp),sizeof(short));
                if(!m_ifFileReader.eof())
                {
                    EndianSwap<short>(&shTemp);
                    m_vShBinValues.push_back(shTemp);
                }
            }
        }
        
        return m_vShBinValues;
    }
 
    template <class T>
    void FileReader::EndianSwap(T *objT)
    {
        LOG_TRACE(__FUNCTION__);

        unsigned char *ptrchTemp = reinterpret_cast<unsigned char*>(objT);
        std::reverse(ptrchTemp, ptrchTemp + sizeof(T));
    }
        
    //////////////////////////////////////////////////
    ///FileWriter
    //////////////////////////////////////////////////
    FileWriter::FileWriter(string _strFilePath)   
    {
        LOG_TRACE(__FUNCTION__);

        this->m_strFilePath = _strFilePath;
    }

    FileWriter::~FileWriter()
    {
        LOG_TRACE(__FUNCTION__);

        if(IsOpen())
            m_ofFileWriter.close();
    }

    bool FileWriter::OpenFile(bool bBinary)
    {
        LOG_TRACE(__FUNCTION__);

        try
        {
            m_ofFileWriter.open(m_strFilePath,ios::out);
            return true;
        }
        catch(...)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"File cannot open!");
            return false;
        }
    }

    bool FileWriter::IsOpen()
    {
        LOG_TRACE(__FUNCTION__);

        return m_ofFileWriter.is_open();
    }

    void FileWriter::CloseFile()
    {
        LOG_TRACE(__FUNCTION__);

        m_ofFileWriter.close();
    }

    void FileWriter::WriteDataToFile()
    {
        LOG_TRACE(__FUNCTION__);

        for(vector<string>::iterator it=(this->m_vStrContent).begin();it!=(this->m_vStrContent).end();it++)
            m_ofFileWriter<<(*it)<<"\n";
    }
}///end of namespace DetCommonNS
