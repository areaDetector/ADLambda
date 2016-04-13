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

#ifndef __NETWORK_IMPLEMENTATION_H__
#define __NETWORK_IMPLEMENTATION_H__

#include "Globals.h"

namespace DetCommonNS
{
 
    /**
     * @brief network implementation
     * This class provides the implmentation for the NetworkInterface @see Network_ns::NetworkInterface
     */
    class NetworkImplementation
    {
      public:
        ///@see NetworkInterface::Connect()
        virtual int Connect() = 0;

        ///@see NetworkInterface::Disconnect()
        virtual int Disconnect() = 0;

        ///@see NetworkInterface::Live()
        virtual bool Live() = 0;

        ///@see NetworkInterface::SendData(char*,int)
        virtual int SendData(char* ptrChData,int nLength) = 0;

        ///@see NetworkInterface::SendData(vector<unsigned char>)
        virtual int SendData(vector<unsigned char> vChData) = 0;

        ///@see NetworkInterface::ReceivePacket(char*,int&)
        virtual int ReceivePacket(char* ptrchData,int& nLength) = 0;
        
        ///@see NetworkInterface::ReceiveData(char*)
        virtual int ReceiveData(char* ptrChData,int nLength) = 0;
            
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
        NetworkImplementation(string _strIP,int _nPort);

        /**
         * @brief get protocol used for network
         * @param enumProtocol TCP,UDP
         */
        int GetProtocol(Enum_protocol enumProtocol) const;
            
      protected:
        string m_strIP;
        int m_nPort;
        bool m_bLive;                      
        int m_nSockfd;
        struct sockaddr_in m_stServAddr;
        struct hostent* m_stServer;   
    }; ///end of class NetworkImplementation    

    /**
     * @brief TCP implementation
     * This class provides the TCP implementation for the NetworkTCPInterface @see Network_ns::NetworkTCPInterface
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
        NetworkTCPImplementation(string _strIP,int _nPort);
            
        /**
         * @brief destructor 
         */
        ~NetworkTCPImplementation();

        ///@see NetworkTCPInterface::Connnect()
        int Connect();

        ///@see NetworkTCPInterface::Disconnect()
        int Disconnect();

        ///@see NetworkTCPInterface::Live()
        bool Live();

        ///@see NetworkTCPInterface::SendData(char*,int)
        int SendData(char* ptrchData,int nLength);
        
        ///@see NetworkTCPInterface::SendData(vector<unsigned char>)
        int SendData(vector<unsigned char> vChData);
        
        ///@see NetworkInterface::ReceivePacket(char*,int&)
        int ReceivePacket(char* ptrchData,int& nLength);

        ///@see NetworkTCPInterface::ReceiveData(char*)
        int ReceiveData(char* ptrchData,int nLength);
    };///end of class NetworkTCPImplementation
        

    /**
     * @brief UDP implementation
     * This class provides the UDP implementation for the NetworkUDPInterface @see Network_ns::NetworkUDPInterface
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
        NetworkUDPImplementation(string _strIP,int _nPort);

        /**
         * @brief destructor
         */
        ~NetworkUDPImplementation();

        ///@see NetworkUDPInterface::Connect()
        int Connect();

        ///@see NetworkUDPInterface::Disconnet()
        int Disconnect();

        ///@see NetworkUDPInterface::Live()
        bool Live();

        ///@see NetworkUDPInterface::SendData(char*,int)
        int SendData(char* ptrchData,int nLength);

        ///@see NetworkTCPInterface::SendData(vector<unsigned char>)
        int SendData(vector<unsigned char> vChData);
        
        ///@see NetworkInterface::ReceivePacket(char*,int&)
        int ReceivePacket(char* ptrchData,int& nLength);
        
        ///@see NetworkUDPInterface::ReceiveData(char*)
        int ReceiveData(char* ptrchData,int nLength);
            
      private:
        bool HasData();
        int HasDataSize();
    };///end of class NetworkUDPImplementation
    
}///end of namespace DetCommonNS


#endif
