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

#include "Compression.h"
#include "ZlibWrapper.h"

///namespace 
namespace CompressionNS
{
    string CompressionInterface::GetErrorMessage()
    {
        string strTmp = m_strErrMsg;
        m_strErrMsg.clear();
        return strTmp;
    }
    
    
    CompressionZlib::CompressionZlib()
        :m_sptrZlibWrapper(new ZlibWrapper())
    {
        
    }    
    
    CompressionZlib::~CompressionZlib()
    {
    }
    
    bool CompressionZlib::CompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData,int nLevel)
    {
        //check compression level, must be between 0-9
        if(nLevel < 0 || nLevel > 9)
        {
            m_strErrMsg = string("Compression level is not correct. It must be between 0-9.");
            return false;
        }
        else
        {
            //do compression
            int nRetVal = m_sptrZlibWrapper->Compress(vuchSrcData,vuchDstData,nLevel);
            if(nRetVal != 0)
            {
                m_strErrMsg = string("Error occurred during compression.");
                return false;
            }
            return true;
        }
    }
    


    bool CompressionZlib::DecompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData)
    {
        //TODO:
    }
    

}///end of namespace
