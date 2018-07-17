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

#pragma once

#include "Globals.h"

namespace FSDetCoreNS
{
    /**
     * @brief network implementation
     * This class provides the implmentation for the NetworkInterface
     * @see Network_ns::NetworkInterface
     */
    class NetworkImplementation
    {
    public:
        ///@see NetworkInterface::Connect()
        virtual int32 Connect() = 0;

        ///@see NetworkInterface::Disconnect()
        virtual int32 Disconnect() = 0;

        ///@see NetworkInterface::Live()
        virtual bool Live() = 0;

        virtual int32 SendData(char* ptrChData, szt nLength) = 0;

        virtual int32 SendData(vector<uchar> vChData) = 0;

        virtual int32 ReceivePacket(char* ptrchData, szt& nLength) = 0;

        virtual int32 ReceiveData(char* ptrChData, szt nLength) = 0;

        virtual int32 ReceiveData(char* ptrChData,
                                  szt nLengthMin,
                                  szt nLengthMax,
                                  szt& nTotalReceived) = 0;

        virtual void ClearDataInSocket() = 0;
        
        /**
         * @brief destructor
         */
        virtual ~NetworkImplementation();

    protected:
        /**
         * @brief constructor
         */
        NetworkImplementation();

        /**
         * @brief constructor
         * @param _strIP ip address
         * @param _nPort port number
         */
        NetworkImplementation(string _strIP, int32 _nPort);

        /**
         * @brief get protocol used for network
         * @param enumProtocol TCP,UDP
         */
        int32 GetProtocol(Enum_protocol enumProtocol) const;

    protected:
        string m_strIP;
        int32 m_nPort;
        bool m_bLive;
        int32 m_nSockfd;
        struct sockaddr_in m_stServAddr;
        struct hostent* m_stServer;
    };    /// end of class NetworkImplementation

    /**
     * @brief TCP implementation
     * This class provides the TCP implementation for the NetworkTCPInterface
     * @see Network_ns::NetworkTCPInterface
     */
    class NetworkTCPImplementation : public NetworkImplementation
    {
    public:
        /**
         * @brief constructor
         */
        NetworkTCPImplementation();

        /**
         * @brief constructor
         * @param _strIP ip address
         * @param _nPort port number
         */
        NetworkTCPImplementation(string strIP, int32 nPort);

        /**
         * @brief destructor
         */
        ~NetworkTCPImplementation();

        ///@see NetworkTCPInterface::Connnect()
        int32 Connect();

        ///@see NetworkTCPInterface::Disconnect()
        int32 Disconnect();

        ///@see NetworkTCPInterface::Live()
        bool Live();

        int32 SendData(char* ptrchData, szt nLength);

        int32 SendData(vector<uchar> vChData);

        int32 ReceivePacket(char* ptrchData, szt& nLength);

        int32 ReceiveData(char* ptrchData, szt nLength);

        int32 ReceiveData(char* ptrchData, szt nLengthMin, szt nLengthMax, szt& nTotalReceived);

        virtual void ClearDataInSocket();

    private:
        bool HasData();

    };


    /**
     * @brief UDP implementation
     * This class provides the UDP implementation for the NetworkUDPInterface
     * @see Network_ns::NetworkUDPInterface
     */
    class NetworkUDPImplementation : public NetworkImplementation
    {
    public:
        /**
         * @brief constructor
         */
        NetworkUDPImplementation();

        /**
         * @brief constructor
         * @param _strIP ip address
         * @param _nPort port number
         */
        NetworkUDPImplementation(string strIP, int32 nPort);

        /**
         * @brief destructor
         */
        ~NetworkUDPImplementation();

        ///@see NetworkUDPInterface::Connect()
        int32 Connect();

        ///@see NetworkUDPInterface::Disconnet()
        int32 Disconnect();

        ///@see NetworkUDPInterface::Live()
        bool Live();

        int32 SendData(char* ptrchData, szt nLength);

        int32 SendData(vector<uchar> vChData);

        int32 ReceivePacket(char* ptrchData, szt& nLength);

        int32 ReceiveData(char* ptrchData, szt nLength);

        // NOT YET IMPLEMENTED
        int32 ReceiveData(char* ptrchData, szt nLengthMin, szt nLengthMax, szt& nTotalReceived);

        virtual void ClearDataInSocket();

    private:
        bool HasData();
        int32 HasDataSize();
    };

}
