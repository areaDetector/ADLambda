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

#include "NetworkInterface.h"
#include "NetworkImplementation.h"

namespace FSDetCoreNS
{
    //////////////////////////////////////////////////
    /// NetworkInterface
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
    /// NetworkTCPInterface
    //////////////////////////////////////////////////
    NetworkTCPInterface::NetworkTCPInterface(NetworkImplementation* _objNetworkImpl)
    : m_objNetworkImpl(_objNetworkImpl)
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkTCPInterface::~NetworkTCPInterface()
    {
        LOG_TRACE(__FUNCTION__);
        delete m_objNetworkImpl;
        m_objNetworkImpl = nullptr;
    }

    int32 NetworkTCPInterface::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Connect();
    }

    int32 NetworkTCPInterface::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Disconnect();
    }

    bool NetworkTCPInterface::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Live();
    }

    int32 NetworkTCPInterface::SendData(char* ptrChData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->SendData(ptrChData, nLength);
    }

    int32 NetworkTCPInterface::SendData(vector<uchar> vChData)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->SendData(vChData);
    }

    int32 NetworkTCPInterface::ReceivePacket(char* ptrchData, szt& nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceivePacket(ptrchData, nLength);
    }

    int32 NetworkTCPInterface::ReceiveData(char* ptrChData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceiveData(ptrChData, nLength);
    }

    int32 NetworkTCPInterface::ReceiveData(char* ptrChData,
                                         szt nLengthMin,
                                         szt nLengthMax,
                                         szt& nTotalReceived)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceiveData(ptrChData, nLengthMin, nLengthMax, nTotalReceived);
    }

    void NetworkTCPInterface::ClearDataInSocket()
    {
        LOG_TRACE(__FUNCTION__);

        m_objNetworkImpl->ClearDataInSocket();
    }

    //////////////////////////////////////////////////
    /// NetworkUDPInterface
    //////////////////////////////////////////////////
    NetworkUDPInterface::NetworkUDPInterface(NetworkImplementation* _objNetworkImpl)
    : m_objNetworkImpl(_objNetworkImpl)
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkUDPInterface::~NetworkUDPInterface()
    {
        LOG_TRACE(__FUNCTION__);
        delete m_objNetworkImpl;
        m_objNetworkImpl = nullptr;
    }

    int32 NetworkUDPInterface::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Connect();
    }

    int32 NetworkUDPInterface::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Disconnect();
    }

    bool NetworkUDPInterface::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->Live();
    }

    int32 NetworkUDPInterface::SendData(char* ptrChData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->SendData(ptrChData, nLength);
    }

    int32 NetworkUDPInterface::SendData(vector<uchar> vChData)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->SendData(vChData);
    }

    int32 NetworkUDPInterface::ReceivePacket(char* ptrchData, szt& nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceivePacket(ptrchData, nLength);
    }

    int32 NetworkUDPInterface::ReceiveData(char* ptrChData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceiveData(ptrChData, nLength);
    }

    int32 NetworkUDPInterface::ReceiveData(char* ptrChData,
                                         szt nLengthMin,
                                         szt nLengthMax,
                                         szt& nTotalReceived)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objNetworkImpl->ReceiveData(ptrChData, nLengthMin, nLengthMax, nTotalReceived);
    }

    void NetworkUDPInterface::ClearDataInSocket()
    {
        LOG_TRACE(__FUNCTION__);

        m_objNetworkImpl->ClearDataInSocket();
    }

}
