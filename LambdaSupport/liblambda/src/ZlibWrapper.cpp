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


#include "ZlibWrapper.h"

///namespace 
namespace CompressionNS
{
    ZlibWrapper::ZlibWrapper()
        :m_stZS()
    {
    }
    
    ZlibWrapper::~ZlibWrapper()
    {
    }
    
    void ZlibWrapper::SetNextIn(vector<unsigned char> vuchNextIn)
    {
        m_stZS.next_in = reinterpret_cast<unsigned char*>(&vuchNextIn[0]);;
    }
    
    void ZlibWrapper::SetNextInBytes(unsigned int unAvailableInBytes)
    {
        m_stZS.avail_in = unAvailableInBytes;
    }
    
    void ZlibWrapper::SetNextOut(vector<unsigned char> vuchNextOut)
    {
        m_stZS.next_out = reinterpret_cast<unsigned char*>(&vuchNextOut[0]);
    }
    
    void ZlibWrapper::SetNextOutBytes(unsigned int unAvailableOutBytes)
    {
        m_stZS.avail_out = unAvailableOutBytes;
    }

    unsigned int ZlibWrapper::GetNextOutBytes() const
    {
        return m_stZS.avail_out;
    }
    

    string ZlibWrapper::GetErrorMsg()
    {
        return string(m_stZS.msg);
    }
    
    void ZlibWrapper::ResetZAlloc()
    {
        m_stZS.zalloc = NULL;
    }
    
    void ZlibWrapper::ResetZFree()
    {
        m_stZS.zfree = NULL;
    }
    
    void ZlibWrapper::ResetOpaque()
    {
        m_stZS.opaque = NULL;
    }
    
    int ZlibWrapper::DeflateInit(int nLevel)
    {
        return deflateInit(&m_stZS,nLevel);
    }
    
    int ZlibWrapper::Deflate(int nFlush)
    {
        //return deflate(&m_stZS,Z_NO_FLUSH);
        return deflate(&m_stZS,nFlush);
    }
    
    int ZlibWrapper::DeflateEnd()
    {
        return deflateEnd(&m_stZS);
    }
    
    int ZlibWrapper::InflateInit()
    {
        return inflateInit(&m_stZS);
    }
    
    int ZlibWrapper::Inflate()
    {
        return inflate(&m_stZS,Z_NO_FLUSH);
    }
    
    int ZlibWrapper::InflateEnd()
    {
        return inflateEnd(&m_stZS);
    }

    int ZlibWrapper::Compress(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData, int nLevel)
    {
        
        int nSrcSize = vuchSrcData.size();
        int nDstSize = DeflateSizeAdjust(nSrcSize);
        uLongf ulfDstSize = (uLongf)nDstSize;
        vuchDstData.resize(nDstSize);
        
        unsigned char* uptrSrcData = reinterpret_cast<unsigned char*>(&vuchSrcData[0]);
        unsigned char* uptrDstData = reinterpret_cast<unsigned char*>(&vuchDstData[0]);

        int nRetVal = compress2(uptrDstData,&(ulfDstSize),uptrSrcData,nSrcSize,nLevel);

        nDstSize = ulfDstSize;
        
        vuchDstData.resize(nDstSize);   
        return nRetVal;
    }

    int ZlibWrapper::DeflateSizeAdjust(int nSize)
    {
        return (ceil(((double)(nSize))*1.001)+12);
    }
    
    
}///end of namespace
