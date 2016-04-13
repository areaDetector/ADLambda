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

#include "NetworkInterface.h"
#include "NetworkImplementation.h"

namespace DetCommonNS
{
    //////////////////////////////////////////////////
    ///NetworkInterface 
    //////////////////////////////////////////////////
    NetworkInterface::NetworkInterface()
    {
        LOG_TRACE(__FUNCTION__);
    }
    
    NetworkInterface::~NetworkInterface()
    {
        LOG_TRACE(__FUNCTION__);
    }

    //////////////////////////////////////////////////
    ///NetworkTCPInterface 
    //////////////////////////////////////////////////
    NetworkTCPInterface::NetworkTCPInterface(NetworkImplementation* _objNetworkImpl)
        :m_objNetworkImpl(_objNetworkImpl)
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    NetworkTCPInterface::~NetworkTCPInterface()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    int NetworkTCPInterface::Connect()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Connect();
    }

    int NetworkTCPInterface::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Disconnect();
    }

    bool NetworkTCPInterface::Live()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Live();
    }
        
    int NetworkTCPInterface::SendData(char* ptrChData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->SendData(ptrChData,nLength);
    }
  
    int NetworkTCPInterface::SendData(vector<unsigned char> vChData)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->SendData(vChData);
    }

    int NetworkTCPInterface::ReceivePacket(char* ptrchData,int& nLength)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->ReceivePacket(ptrchData,nLength);
    }
    
    int NetworkTCPInterface::ReceiveData(char* ptrChData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->ReceiveData(ptrChData,nLength);
    }
        
    //////////////////////////////////////////////////
    ///NetworkUDPInterface 
    //////////////////////////////////////////////////
    NetworkUDPInterface::NetworkUDPInterface(NetworkImplementation* _objNetworkImpl)
        :m_objNetworkImpl(_objNetworkImpl)
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkUDPInterface::~NetworkUDPInterface()
    {
        LOG_TRACE(__FUNCTION__);
    }

    int NetworkUDPInterface::Connect()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Connect();
    }

    int NetworkUDPInterface::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Disconnect();
    }

    bool NetworkUDPInterface::Live()
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->Live();
    }
        
    int NetworkUDPInterface::SendData(char* ptrChData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->SendData(ptrChData,nLength);
    }

    int NetworkUDPInterface::SendData(vector<unsigned char> vChData)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->SendData(vChData);
    }

    int NetworkUDPInterface::ReceivePacket(char* ptrchData,int& nLength)
    {
        LOG_TRACE(__FUNCTION__);
        
        return m_objNetworkImpl->ReceivePacket(ptrchData,nLength);
    }

    int NetworkUDPInterface::ReceiveData(char* ptrChData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
            
        return m_objNetworkImpl->ReceiveData(ptrChData,nLength);
    }   
}//end of namespace DetCommonNS

