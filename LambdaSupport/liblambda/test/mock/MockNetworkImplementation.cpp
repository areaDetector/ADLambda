
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

#include "MockNetworkImplementation.h"

namespace DetCommonNS
{
    //////////////////////////////////////////////////
    ///NetworkImplementation 
    //////////////////////////////////////////////////
    NetworkImplementation::NetworkImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkImplementation::NetworkImplementation(string _strIP,int _nPort)
        :m_strIP(_strIP),m_nPort(_nPort),m_bLive(false),m_nSockfd(-1),m_stServer(NULL)
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    NetworkImplementation::~NetworkImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    int NetworkImplementation::GetProtocol(Enum_protocol enumProtocol) const
    {
        LOG_TRACE(__FUNCTION__);
        
        switch(enumProtocol)
        {
            case TCP:
                return SOCK_STREAM;
                break;
            case UDP:
                return SOCK_DGRAM;
                break;
            default:
                LOG_STREAM(__FUNCTION__,ERROR,"unkown protocol type!");
                return -1;
        }
            
    }

    vector<char> NetworkImplementation::GetData()
    {   
        return m_vchData;
    }

    void NetworkImplementation::ClearData()
    {
        // cout<<m_vchData.size()<<endl<<"========================="<<endl;
        // for(vector<char>::iterator it=m_vchData.begin();it!=m_vchData.end();it++)
        //     cout<<hex<<m_vchData<<endl;
        // cout<<"========================="<<endl;
        m_vchData.clear();
        
    }
    
    
    //////////////////////////////////////////////////
    ///NetworkTCPImplementation 
    //////////////////////////////////////////////////      
    NetworkTCPImplementation::NetworkTCPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkTCPImplementation::NetworkTCPImplementation(string _strIP,int _nPort)
    {
        LOG_TRACE(__FUNCTION__);
        
        this->m_strIP = _strIP;
        this->m_nPort = _nPort;
        this->m_bLive = false;
        this->m_nSockfd = -1;
        this->m_stServer = NULL;
    }
        
    NetworkTCPImplementation::~NetworkTCPImplementation()
    {
        
    }
       
    int NetworkTCPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);

    }
        
    int NetworkTCPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);
    }

    bool NetworkTCPImplementation::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_bLive;
    }

    int NetworkTCPImplementation::ReceivePacket(char* ptrchData,int& nLength)
    {
        LOG_TRACE(__FUNCTION__);
    }

    int NetworkTCPImplementation::ReceiveData(char* ptrchData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
    }

    int NetworkTCPImplementation::SendData(char* ptrchData,int nLength)
    {
        m_vchData.insert(m_vchData.end(),ptrchData,ptrchData+nLength);
    }
        
    int NetworkTCPImplementation::SendData(vector<unsigned char> vChData)
    {
        int nSize = vChData.size();
        char* ptrchData = reinterpret_cast<char*>(&vChData[0]);
        m_vchData.insert(m_vchData.end(),ptrchData,ptrchData+nSize);
    }

    //////////////////////////////////////////////////
    ///NetworkUDPImplementation 
    //////////////////////////////////////////////////   
    NetworkUDPImplementation::NetworkUDPImplementation()
    {
        LOG_TRACE(__FUNCTION__);               
    }

    NetworkUDPImplementation::NetworkUDPImplementation(string _strIP,int _nPort)
    {
        LOG_TRACE(__FUNCTION__);
        
        this->m_strIP = _strIP;
        this->m_nPort = _nPort;
        this->m_bLive = false;
        this->m_nSockfd = -1;
        this->m_stServer = NULL;
    }
        
    NetworkUDPImplementation::~NetworkUDPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    int NetworkUDPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);
    }

    int NetworkUDPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);
    }

    bool NetworkUDPImplementation::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_bLive;
    }

    bool NetworkUDPImplementation::HasData()
    {
        // LOG_TRACE(__FUNCTION__);
    }

    int NetworkUDPImplementation::HasDataSize()
    {
        // LOG_TRACE(__FUNCTION__);
    }


    int NetworkUDPImplementation::ReceivePacket(char* ptrchData,int& nLength)
    {  
    }
    
    int NetworkUDPImplementation::ReceiveData(char* ptrchData,int nLength)
    {
    }

    int NetworkUDPImplementation::SendData(char* ptrchData,int nLength)
    { 
    }        

    int NetworkUDPImplementation::SendData(vector<unsigned char> vChData)
    {
    }
    
   
}///end of namespace DetCommonNS
