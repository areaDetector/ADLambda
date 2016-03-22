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

#ifndef __NETWORK_INTERFACE_H__
#define __NETWORK_INTERFACE_H__

#include "Globals.h"

///namespace DetCommonNS
namespace DetCommonNS
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
        virtual int Connect() = 0;

        /**
         * @brief close network connection interface
         * @return error code
         */
        virtual int Disconnect() = 0;

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
        virtual int SendData(char* ptrChData,int nLength) = 0;

        /**
         * @brief send data to detector via network overloaded
         * @param vChData data
         * @return error code
         */
        virtual int SendData(vector<unsigned char> vChData) = 0;

        /**
         * @brief receive packet
         * @param ptrChData data
         * @return length of data
         */
        virtual int ReceivePacket(char* ptrchData,int& nLength) = 0;
        
        /**
         * @brief receive data from detector via network
         * @param ptrChData data
         * @param nLength length of required data
         * @return error code
         */
        virtual int ReceiveData(char* ptrChData,int nLength) = 0;
            
        /**
         * @brief destructor
         */
        virtual ~NetworkInterface();
            
      protected:
        /**
         * @brief constructor
         */
        NetworkInterface();    
    };///end of class NetworkInterface


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
        virtual int Connect();

        ///@see NetworkInterface::Disconnect()
        virtual int Disconnect();

        ///@see NetworkInterface::Live()
        virtual bool Live();

        ///@see NetworkInterface::SendData(char*,int)
        virtual int SendData(char* ptrChData,int nLength);
        
        ///@see NetworkInterface::SendData(vector<unsigned char>)
        virtual int SendData(vector<unsigned char> vChData);

        ///@see NetworkInterface::ReceivePacket(char*,int&)
        virtual int ReceivePacket(char* ptrchData,int& nLength);

        ///@see NetworkInterface::ReceiveData(char*,int)
        virtual int ReceiveData(char* ptrChData,int nLength);
            
      private:
        NetworkImplementation* m_objNetworkImpl;
    }; ///end of class NetworkTCPInterface

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
        virtual int Connect();

        ///@see NetworkInterface::Disconnect
        virtual int Disconnect();

        ///@see NetworkInterface::Live
        virtual bool Live();

        ///@see NetworkInterface::SendData(char*,int)
        virtual int SendData(char* ptrChData,int nLength);
        
        ///@see NetworkInterface::SendData(vector<unsigned char>)
        virtual int SendData(vector<unsigned char> vChData);

        ///@see NetworkInterface::ReceivePacket(char*,int&)
        virtual int ReceivePacket(char* ptrchData,int& nLength);
        
        ///@see NetworkInterface::ReceiveData(char*,int)
        virtual int ReceiveData(char* ptrChData,int nLength);
            
      private:
        NetworkImplementation* m_objNetworkImpl;
    };///end of class NetworkUDPInterface
}///end of namespace DetCommonNS

#endif
