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
    class NetworkImplementation;
    /**
     * @brief network interface class
     * This class provides all network interfaces used for  system library.
     */
    class NetworkInterface
    {
    public:
        /**
         * @brief connect socket interface
         * @return error code
         */
        virtual int32 Connect() = 0;

        /**
         * @brief close network connection interface
         * @return error code
         */
        virtual int32 Disconnect() = 0;

        /**
         *@brief check if the connection is still valid
         *@return status of the connection. True: connected; false: not connected.
         */
        virtual bool Live() = 0;

        /**
         * @brief send data to detector via network
         * @param ptrChData data
         * @param nLength length of the data
         * @return error code
         */
        virtual int32 SendData(char* ptrChData, szt nLength) = 0;

        /**
         * @brief send data to detector via network overloaded
         * @param vChData data
         * @return error code
         */
        virtual int32 SendData(vector<uchar> vChData) = 0;

        /**
         * @brief receive packet
         * @param ptrChData data
         * @return length of data
         */
        virtual int32 ReceivePacket(char* ptrchData, szt& nLength) = 0;

        /**
         * @brief receive data from detector via network
         * @param ptrChData data
         * @param nLength length of required data
         * @return error code
         */
        virtual int32 ReceiveData(char* ptrChData, szt nLength) = 0;

        /**
         * @brief receive data from detector via network
         * @param ptrChData data
         * @param nLengthMin - minimum data length expected -
         *        we will wait until we receive this much data
         * @param nLengthMax - maximum data length allowed -
         *        if more data is in the buffer we will receive up to this value
         * @param nTotalReceived - returns number of bytes received
         * @return error code
         */
        virtual int32 ReceiveData(char* ptrChData,
                                  szt nLengthMin,
                                  szt nLengthMax,
                                  szt& nTotalReceived) = 0;

        /**
         * @brief clear the old data in the socket
         */
        virtual void ClearDataInSocket() = 0;
        
        /**
         * @brief destructor
         */
        virtual ~NetworkInterface();

    protected:
        /**
         * @brief constructor
         */
        NetworkInterface();
    };


    /**
     * @brief TCP interface
     */
    class NetworkTCPInterface : public NetworkInterface
    {
    public:
        /**
         * @brief constructor
         * @param _objNetworkImpl implementation
         */
        NetworkTCPInterface(NetworkImplementation* _objNetworkImpl);

        /**
         * @brief destructor
         */
        virtual ~NetworkTCPInterface();

        ///@see NetworkInterface::Connect()
        virtual int32 Connect();

        ///@see NetworkInterface::Disconnect()
        virtual int32 Disconnect();

        ///@see NetworkInterface::Live()
        virtual bool Live();

        virtual int32 SendData(char* ptrChData, szt nLength);

        virtual int32 SendData(vector<uchar> vChData);

        virtual int32 ReceivePacket(char* ptrchData, size_t& nLength);

        virtual int32 ReceiveData(char* ptrChData, szt nLength);

        virtual int32 ReceiveData(char* ptrChData,
                                  szt nLengthMin,
                                  szt nLengthMax,
                                  szt& nTotalReceived);

        virtual void ClearDataInSocket();
        

    private:
        NetworkImplementation* m_objNetworkImpl;
    };

    /**
     * @brief UDP interface
     */
    class NetworkUDPInterface : public NetworkInterface
    {
    public:
        /**
         * @brief constructor
         * @param _objNetworkImpl implementation
         */
        NetworkUDPInterface(NetworkImplementation* _objNetworkImpl);
        /**
         * @brief destructor
         */
        virtual ~NetworkUDPInterface();

        ///@see NetworkInterface::Connect()
        virtual int32 Connect();

        ///@see NetworkInterface::Disconnect
        virtual int32 Disconnect();

        ///@see NetworkInterface::Live
        virtual bool Live();

        virtual int32 SendData(char* ptrChData, szt nLength);

        virtual int32 SendData(vector<unsigned char> vChData);

        virtual int32 ReceivePacket(char* ptrchData, szt& nLength);

        virtual int32 ReceiveData(char* ptrChData, szt nLength);

        /// NOT YET IMPLEMENTED - NEEDED TO BE ADDED TO ALLOW TCP LINK TO HAVE EQUIVALENT FUNCTION
        virtual int32 ReceiveData(char* ptrChData,
                                  szt nLengthMin,
                                  szt nLengthMax,
                                  szt& nTotalReceived);

        virtual void ClearDataInSocket();

    private:
        NetworkImplementation* m_objNetworkImpl;
    };
}
