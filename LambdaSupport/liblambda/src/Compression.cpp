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

#include "Compression.h"
#include "ZlibWrapper.h"

namespace DetLambdaNS
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

    bool CompressionZlib::CompressData(vector<uchar>& vuchSrcData,
                                       vector<uchar>& vuchDstData,
                                       szt nLevel)
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



    bool CompressionZlib::DecompressData(vector<uchar>& vuchSrcData,
                                         vector<uchar>& vuchDstData)
    {
        //TODO:
        return false;
    }


    #ifdef ENABLEHWCOMPRESSION
    //////////////////////////////////////////////////
    /// CompressionHWAHA
    //////////////////////////////////////////////////
    CompressionHWAHA::CompressionHWAHA(int32 nChunkIn,int32 nChunkOut,uint8 unCtrl,int32 nPostCode)
        :m_nBoard(-1),
         m_nChunkIn(nChunkIn),
         m_nChunkOut(nChunkOut),
         m_nPostCode(nPostCode),
         m_nDataLength(0),
         m_nProcessed(0),
         m_nTotalSize(0),
         m_unType(0),//compression
         m_unCtrl(unCtrl)
    {

        for(szt i=0; i<2; i++)
        {
            auto output = new uchar[nChunkOut];
            m_vOutBuffer.push_back(output);
        }

        if(Open() != 0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"cannot open the compression card");
            Close();
        }
    }

    CompressionHWAHA::~CompressionHWAHA()
    {
        for(auto output : m_vOutBuffer)
            delete[] output;

        Close();
    }

    bool CompressionHWAHA::CompressData(vector<uchar>& vuchSrcData,
                                        vector<uchar>& vuchDstData,
                                        szt nLevel)
    {
        int32 nRet;
        int32 nCollectedBytes = 0;
        uint32 unIn = 1; // buffer is one
        uint32 unOut = 1;

        m_nTotalSize  =  static_cast<int32>(vuchSrcData.size());

        if(!vuchDstData.empty())
            vuchDstData.clear();

        m_nProcessed = 0;

        m_ptrDataSrc = reinterpret_cast<uchar*>(&vuchSrcData[0]);
        AddInputBuffer();
        AddOutputBuffer();

        do
        {
            nRet = ahagz_api_waitstat(&m_as, &unIn, &unOut, 30000);

            if( nRet == RET_BUFF_RECLAIM)
            {
                if( unIn > 0 )
                {
                    AddInputBuffer();

                    unIn--;
                }

                if( unOut > 0)
                {
                    vuchDstData.insert(vuchDstData.end(),m_vOutBuffer[0],m_vOutBuffer[0] + m_nChunkOut);

                    nCollectedBytes += m_nChunkOut;

                    AddOutputBuffer();

                    unOut--;
                }
            }
            else
            {
                if( nRet  != 0x8000)
                {
                    LOG_STREAM(__FUNCTION__,ERROR,"unexpected error" + to_string(nRet));
                    return false;
                }
            }
        }while(nRet < 0x8000);

        if((nRet = ahagz_api_output_size(&m_as)) < 0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"output size is wrong");
            return false;
        }
        else
        {
            if(nRet - nCollectedBytes < 0)
            {
                LOG_STREAM(__FUNCTION__,ERROR,"total compressed bytes is less than already stored bytes");
                return false;
            }
            else
            {
                vuchDstData.insert(vuchDstData.end(),m_vOutBuffer[0],m_vOutBuffer[0] + nRet - nCollectedBytes);
                // vuchDstData.insert(
                //     vuchDstData.end(),m_vOutBuffer[0],m_vOutBuffer[0] + nRet nCollectedBytes);
                // cout<<"--------------------------------------------------"
                //     <<"\ntotal output size is : "<< vuchDstData.size()
                //     <<"\napi output size : "<<nRet
                //     <<"\noriginal data size : "<<m_nTotalSize
                //     <<"\ncompression ration : "<<m_nTotalSize/nRet<< endl;
                //return true;
            }
        }

        if(ahagz_api_reinitialize(&m_as,m_unCtrl,1))
            LOG_STREAM(__FUNCTION__,ERROR,"cannot reinitialize the card");

        return true;
    }

    bool CompressionHWAHA::DecompressData(vector<uchar>& vuchSrcData,
                                          vector<uchar>& vuchDstData)
    {
        return false;
    }

    int32 CompressionHWAHA::Open()
    {
        int32  nRet;
        //m_unCtrl = 0x2; //deflate

        //open device
        if(m_nBoard < 0)
            nRet = ahagz_api_open(&m_as,m_unType,m_unCtrl);
        else
            nRet = ahagz_api_open_select(&m_as,m_unType,m_unCtrl,m_nBoard,-1);
        if(nRet < 0)
            LOG_STREAM(__FUNCTION__,ERROR,"open compression card error");

        if(nRet == 0)
            cout << "open card..." << endl;
        LOG_INFOS("open compression card successfully");
        return nRet;
    }

    int32 CompressionHWAHA::Close()
    {
        LOG_INFOS("compression card closed");

        return ahagz_api_close(&m_as);
    }

    void CompressionHWAHA::AddInputBuffer()
    {
        int32 nRet;

        int32 nRemainSize = m_nTotalSize - m_nProcessed;

        if(m_nChunkIn >= nRemainSize)
        {
            if((nRet = ahagz_api_addinput(&m_as,m_ptrDataSrc,nRemainSize,
                                          (m_nProcessed == 0),1,m_nPostCode,NULL,0)) != 0)
                LOG_INFOS("cannot add last input buffer");

            m_nProcessed += nRemainSize;
            m_ptrDataSrc += nRemainSize;
        }
        else
        {
            if((nRet = ahagz_api_addinput(&m_as,m_ptrDataSrc,m_nChunkIn,
                                          (m_nProcessed == 0),0,m_nPostCode,NULL,0)) != 0)
                LOG_INFOS("cannot add input buffer");

            m_nProcessed += m_nChunkIn;
            m_ptrDataSrc += m_nChunkIn;
        }
    }

    void CompressionHWAHA::AddOutputBuffer()
    {
        int nRet ;

        if((nRet = ahagz_api_addoutput(&m_as, m_vOutBuffer[0], m_nChunkOut)) != 0)
            LOG_INFOS("cannot add output buffer");
    }
    #endif
}
