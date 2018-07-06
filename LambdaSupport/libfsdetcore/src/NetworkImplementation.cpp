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

#include "NetworkImplementation.h"

namespace FSDetCoreNS
{
    //////////////////////////////////////////////////
    /// NetworkImplementation
    //////////////////////////////////////////////////
    NetworkImplementation::NetworkImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkImplementation::NetworkImplementation(string _strIP, int32 _nPort)
    : m_strIP(_strIP)
    , m_nPort(_nPort)
    , m_bLive(false)
    , m_nSockfd(-1)
    , m_stServer(NULL)
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkImplementation::~NetworkImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    int32 NetworkImplementation::GetProtocol(Enum_protocol enumProtocol) const
    {
        LOG_TRACE(__FUNCTION__);

        switch(enumProtocol) {
            case TCP:
                return SOCK_STREAM;
                break;
            case UDP:
                return SOCK_DGRAM;
                break;
            default:
                LOG_STREAM(__FUNCTION__, ERROR, "unkown protocol type!");
                return -1;
        }
    }

    //////////////////////////////////////////////////
    /// NetworkTCPImplementation
    //////////////////////////////////////////////////
    NetworkTCPImplementation::NetworkTCPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkTCPImplementation::NetworkTCPImplementation(string _strIP, int32 _nPort)
    {
        LOG_TRACE(__FUNCTION__);

        this->m_strIP = _strIP;
        this->m_nPort = _nPort;
        this->m_bLive = false;
        this->m_nSockfd = -1;
        this->m_stServer = NULL;

        // int nOptVal =1;
        // int nOptlen;

        /// initialize socket
        int32 nProtocol = GetProtocol(TCP);

        /// create socket
        m_nSockfd = socket(AF_INET, nProtocol, 0);

        // TODO fuerstel: why is this commented out?
        // setsockopt(m_nSockfd,SOL_SOCKET,SO_REUSEADDR,&nOptVal,sizeof(nOptVal));

        if(m_nSockfd < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot open socket!");

        /// try to get host via IP address
        m_stServer = gethostbyname(m_strIP.c_str());
        if(m_stServer == NULL) {
            LOG_STREAM(__FUNCTION__, ERROR, "cannot find host!");
            exit(0);
        }

        int32 nOne = 1;

        if(setsockopt(m_nSockfd, IPPROTO_TCP, TCP_NODELAY, &nOne, sizeof(nOne)) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot set TCP to be NODELAY:" + to_string(errno));

        /// set timeout for receiving data
        /// avoid recvfrom hanging
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        if(setsockopt(m_nSockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot set socket timeout");

        bzero((char*)&m_stServAddr, sizeof(m_stServAddr));
        m_stServAddr.sin_family = AF_INET;
        bcopy((char*)m_stServer->h_addr, (char*)&m_stServAddr.sin_addr.s_addr, static_cast<szt>(m_stServer->h_length));
        m_stServAddr.sin_port = htons(m_nPort);

        LOG_INFOS("TCP initializes successfully.");
    }

    NetworkTCPImplementation::~NetworkTCPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
        Disconnect();
    }

    int32 NetworkTCPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        int32 nRet = connect(m_nSockfd, (struct sockaddr*)&m_stServAddr, sizeof(m_stServAddr));
        if(nRet < 0) {
            LOG_STREAM(__FUNCTION__,
                       ERROR,
                       "TCP cannot connect to host! Unable to connect to detector - please check that it is powered and "
                       "correctly connected");
            return nRet;
        }

        LOG_INFOS("connected to host successfully via TCP protocol.");
        m_bLive = true;
        return nRet;
    }

    int32 NetworkTCPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        close(m_nSockfd);
        return 0;
    }

    bool NetworkTCPImplementation::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_bLive;
    }

    int32 NetworkTCPImplementation::ReceivePacket(char* ptrchData, szt& nLength)
    {
        LOG_TRACE(__FUNCTION__);
        sszt nRetVal = -1;
        szt nReadLen = 1500;

        nRetVal = ::recv(m_nSockfd, (char*)ptrchData, nReadLen, 0);

        //nLength = static_cast<szt>(nRetVal);

        nLength = (nRetVal>0 ? nRetVal:0);

        if(nRetVal > 0)
            return 0;    // Indicates OK
        else
            return -1;    // No data
    }

    int32 NetworkTCPImplementation::ReceiveData(char* ptrchData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        bool bFirstPacket = true;
        int32 nErrorCode = 0;
        szt nReceivedData = 0;
        sszt nRetVal = -1;
        szt nFrameLength = nLength;
        sszt nBytesLeft = static_cast<sszt>(nLength);
        szt nPos = 0;
        int32 nCounter = 0;

        szt nPacketLength = 1500;
        char* chDataRec = new char[nPacketLength];

        while(1)    // We break when the image is finished
        {

            while(!HasData()) {
                usleep(50);    // Don't poll excessively if no data available
            }

            nRetVal = ::recv(m_nSockfd, (char*)chDataRec, nPacketLength, 0);
            // nRetVal has no of bytes, 0 if none, -1 if error
            if(nRetVal < 0)
                continue;    // If error, try again

            if(nRetVal > nBytesLeft) {
                // data already out of bounds;
                nErrorCode += 4;
                LOG_STREAM(__FUNCTION__, ERROR, "Image data is out of bound");
                LOG_INFOS(("Data out of bound:" + to_string(static_cast<long long>(nRetVal)) + "Data remain:"
                           + to_string(static_cast<long long>(nBytesLeft))));
                break;
            }

            nCounter++;

            if(!bFirstPacket) {
                // copy data to destination buffer
                if(nRetVal > 0) {
                    std::copy(chDataRec, chDataRec + nRetVal, ptrchData + nPos);
                    nReceivedData += static_cast<szt>(nRetVal);
                }
                nBytesLeft = nBytesLeft - (nRetVal);
                nPos = nPos + static_cast<szt>(nRetVal);
            } else {
                if(nRetVal > 0) {
                    std::copy(chDataRec, chDataRec + nRetVal, ptrchData);
                    nReceivedData += static_cast<szt>(nRetVal);
                }

                // first byte of the image should be 0xa0
                uchar uchByte = static_cast<uchar>(ptrchData[0]);
                if(uchByte != 0xa0) {
                    LOG_STREAM(__FUNCTION__, ERROR, "Image data is wrong!");
                    nErrorCode += 2;
                }
                bFirstPacket = false;
                nBytesLeft = nBytesLeft - (nRetVal);
                nPos = nPos + static_cast<szt>(nRetVal);
            }

            if(nPos >= nFrameLength) {
                LOG_INFOS("One image is finished.");
                if(nReceivedData < nFrameLength)
                    nErrorCode++;

                break;
            }
        }
        LOG_INFOS(("Total Received Data:" + to_string(static_cast<long long>(nReceivedData)) + "Error code is:"
                   + to_string(static_cast<long long>(nErrorCode))));
        delete[] chDataRec;
        return nErrorCode;
    }

    bool NetworkTCPImplementation::HasData()
    {
        LOG_TRACE(__FUNCTION__);

        sszt nRetVal = -1;

        do {
            char chTemp;
            nRetVal = ::recv(m_nSockfd, &chTemp, 1, MSG_PEEK);
        } while(nRetVal == -1 && errno == EINTR);

        bool bHasData = (nRetVal != -1) || errno == EMSGSIZE;
        return bHasData;
    }


    int32 NetworkTCPImplementation::ReceiveData(char* ptrchData, szt nLengthMin, szt nLengthMax, szt& nTotalReceived)
    {
        LOG_TRACE(__FUNCTION__);
        static const szt BUFLEN = 1500;

        szt nRemainByte = 0;
        sszt nRet = -1;
        szt nReadLen = BUFLEN;
        char ptrChRec[BUFLEN];
        int32 nTry = 0;
        int32 nMaxTries = 10000;
        nTotalReceived = 0;

        while(nTotalReceived < nLengthMax) {
            nRemainByte = nLengthMax - nTotalReceived;
            if(nRemainByte < nReadLen)
                nReadLen = nRemainByte;
            if((nRet = ::recv(m_nSockfd, ptrChRec, nReadLen, 0)) < 0) {
                // If we already received enough data, then we should return without waiting further
                if(nTotalReceived >= nLengthMin)
                    break;
                else {
                    nTry++;
                    if(nTry > nMaxTries) {
                        return -1;
                    }
                }
            } else {
                nTry = 0;
                std::copy(ptrChRec, ptrChRec + nRet, ptrchData + nTotalReceived);
                nTotalReceived += static_cast<szt>(nRet);
            }
        }
        return 0;
    }


    int32 NetworkTCPImplementation::SendData(char* ptrchData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        sszt nRet = -1;
        nRet = ::send(m_nSockfd, ptrchData, nLength, 0);
        if(nRet == -1) {
            LOG_STREAM(__FUNCTION__, ERROR, "send data error!");
        } else {
            if(static_cast<szt>(nRet) == nLength)
                LOG_INFOS("send data successfully.");
        }

        return static_cast<int32>(nRet);
    }

    int32 NetworkTCPImplementation::SendData(vector<uchar> vChData)
    {
        LOG_TRACE(__FUNCTION__);

        sszt nRet = -1;
        szt nSize = vChData.size();
        char* ptrchData = reinterpret_cast<char*>(&vChData[0]);

        // std::stringstream ss;
        // ss << std::hex << std::setfill('0');
        // for(auto c : vChData) {
        //     ss << std::setw(2) << static_cast<unsigned>(c);
        // }
        // std::cerr << "Sending " + ss.str() << std::endl;
        nRet = ::send(m_nSockfd, ptrchData, nSize, 0);
        if(nRet == -1) {
            LOG_STREAM(__FUNCTION__, ERROR, "send data error!");
        } else {
            if(static_cast<szt>(nRet) == nSize)
                LOG_INFOS("send data successfully.");
        }

        return static_cast<int32>(nRet);
    }

    void NetworkTCPImplementation::ClearDataInSocket()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];
        szt nBytesLength;
        int32 nCount = 0;

        while(ReceivePacket(ptrchPacket,nBytesLength) != -1)
        {
            nCount++;
        }

        LOG_INFOS("Get Extra packets:" + to_string(nCount));

        delete [] ptrchPacket;
    }


    //////////////////////////////////////////////////
    /// NetworkUDPImplementation
    //////////////////////////////////////////////////
    NetworkUDPImplementation::NetworkUDPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }

    NetworkUDPImplementation::NetworkUDPImplementation(string _strIP, int32 _nPort)
    {
        LOG_TRACE(__FUNCTION__);

        this->m_strIP = _strIP;
        this->m_nPort = _nPort;
        this->m_bLive = false;
        this->m_nSockfd = -1;
        this->m_stServer = NULL;

        int32 nOptVal = 1;
        /// initialize socket
        int32 nProtocol = GetProtocol(UDP);

        /// create socket
        m_nSockfd = socket(AF_INET, nProtocol, 0);
        setsockopt(m_nSockfd, SOL_SOCKET, SO_REUSEADDR, &nOptVal, sizeof(nOptVal));

        if(m_nSockfd < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot open socket!");

        /// set timeout for receiving data
        /// avoid recvfrom hanging
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        if(setsockopt(m_nSockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot set socket timeout");

        /// set socket buffer size
        int32 nRcvBuffLen;
        socklen_t nRcvLen = sizeof(nRcvBuffLen);
        if(getsockopt(m_nSockfd, SOL_SOCKET, SO_RCVBUF, (void*)&nRcvBuffLen, &nRcvLen) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot get the socket buffer size");
        // printf("the recevice buf len: %d\n", nRcvBuffLen);

        nRcvBuffLen *= 5;
        nRcvLen = sizeof(nRcvBuffLen);

        if(setsockopt(m_nSockfd, SOL_SOCKET, SO_RCVBUF, (void*)&nRcvBuffLen, nRcvLen) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot set the socket buffer size");

        if(getsockopt(m_nSockfd, SOL_SOCKET, SO_RCVBUF, (void*)&nRcvBuffLen, (socklen_t*)&nRcvLen) < 0)
            LOG_STREAM(__FUNCTION__, ERROR, "cannot get the socket buffer size1");

        /// try to get host via IP address
        m_stServer = gethostbyname(m_strIP.c_str());
        if(m_stServer == NULL) {
            LOG_STREAM(__FUNCTION__, ERROR, "UDP cannot find host!");
            exit(0);
        }

        bzero((char*)&m_stServAddr, sizeof(m_stServAddr));
        m_stServAddr.sin_family = AF_INET;
        bcopy((char*)m_stServer->h_addr, (char*)&m_stServAddr.sin_addr.s_addr, static_cast<szt>(m_stServer->h_length));
        m_stServAddr.sin_port = htons(m_nPort);

        LOG_INFOS("UDP initializes successfully.");
    }

    NetworkUDPImplementation::~NetworkUDPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
        Disconnect();
    }

    int32 NetworkUDPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        int32 nRet = ::bind(m_nSockfd, (struct sockaddr*)&m_stServAddr, sizeof(m_stServAddr));
        if(nRet < 0) {
            LOG_STREAM(__FUNCTION__, ERROR, "UDP bind failed!");
            return nRet;
        }

        LOG_INFOS("connected to host successfully via UDP protocol.");
        m_bLive = true;
        return nRet;
    }

    int32 NetworkUDPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        /// close socket
        close(m_nSockfd);
        return 0;
    }

    bool NetworkUDPImplementation::Live()
    {
        LOG_TRACE(__FUNCTION__);

        return m_bLive;
    }

    bool NetworkUDPImplementation::HasData()
    {
        LOG_TRACE(__FUNCTION__);

        struct sockaddr_in stAddr;
        int32 nAddrLen = sizeof(stAddr);
        sszt nRetVal = -1;

        do {
            char chTemp;
            nRetVal = ::recvfrom(m_nSockfd, &chTemp, 1, MSG_PEEK, (struct sockaddr*)&stAddr, (socklen_t*)&nAddrLen);
        } while(nRetVal == -1 && errno == EINTR);

        bool bHasData = (nRetVal != -1) || errno == EMSGSIZE;
        return bHasData;
    }

    int32 NetworkUDPImplementation::HasDataSize()
    {
        LOG_TRACE(__FUNCTION__);

        sszt nRetVal = -1;
        szt nTempSize = 9600;
        char* chData = new char[nTempSize];
        while(true) {
            nRetVal = ::recv(m_nSockfd, chData, nTempSize, MSG_PEEK);
            if(nRetVal == -1 && errno == EINTR)
                continue;
            if(static_cast<szt>(nRetVal) != nTempSize)
                break;

            nTempSize *= 2;
            delete[] chData;
            chData = new char[nTempSize];
        }
        delete[] chData;
        return static_cast<int32>(nRetVal);
    }

    int32 NetworkUDPImplementation::ReceivePacket(char* ptrchData, szt& nLength)
    {
        sszt nRetVal = -1;
        struct sockaddr_in stAddr;
        int32 nAddrLen = sizeof(stAddr);
        szt nMaxSize = 9600;    // For the time being, hard-code a value for test purposes
        nRetVal = ::recvfrom(m_nSockfd, ptrchData, nMaxSize, 0, (struct sockaddr*)&stAddr, (socklen_t*)&nAddrLen);
        nLength = static_cast<szt>(nRetVal);
        if(nRetVal > 0)
            return 0;    // Indicate data
        else
            return -1;    // no data
    }

    int32 NetworkUDPImplementation::ReceiveData(char* ptrchData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);

        bool bFirstPacket = true;
        int32 nErrorCode = 0;
        szt nReceivedData = 0;
        sszt nRetVal = -1;
        szt nFrameLength = nLength;
        sszt nBytesLeft = static_cast<sszt>(nLength);
        szt nPos = 0;
        sszt nBytesAv = 0;

        struct sockaddr_in stAddr;
        int32 nAddrLen = sizeof(stAddr);

        char chDataRec[UDP_PACKET_SIZE_NORMAL];

        while(true)    // We break when the image is finished
        {
            while(!HasData()) {
                usleep(50);    // Don't poll excessively if no data available
            }

            nBytesAv = HasDataSize();    // Check packet size
            if(nBytesAv > nBytesLeft + UDP_EXTRA_BYTES) {
                // data already out of bounds;
                nErrorCode += 4;
                LOG_STREAM(__FUNCTION__, ERROR, "Image data is out of bound");
                LOG_INFOS(("Data out of bound:" + to_string(static_cast<long long>(nBytesAv)) + "Data remain:"
                           + to_string(static_cast<long long>(nBytesLeft))));
                break;
            }

            nRetVal = ::recvfrom(m_nSockfd, chDataRec, static_cast<szt>(nBytesAv), 0, (struct sockaddr*)&stAddr, (socklen_t*)&nAddrLen);

            if(!bFirstPacket) {
                // copy data to destination buffer, needs to skip the UDP_EXTRA_BYTES
                if(nRetVal > 0) {
                    std::copy(chDataRec + UDP_EXTRA_BYTES, chDataRec + nRetVal, ptrchData + nPos);
                    nReceivedData += static_cast<szt>(nRetVal);
                }
                nBytesLeft = nBytesLeft - (nBytesAv - UDP_EXTRA_BYTES);
                nPos = nPos + static_cast<szt>(nBytesAv - UDP_EXTRA_BYTES);
            } else {
                if(nRetVal > 0) {
                    std::copy(chDataRec + UDP_EXTRA_BYTES, chDataRec + nRetVal, ptrchData);
                    nReceivedData += static_cast<szt>(nRetVal);
                }

                // first byte of the image should be 0xa0
                uchar uchByte = static_cast<uchar>(ptrchData[UDP_EXTRA_BYTES]);
                if(uchByte != 0xa0) {
                    LOG_STREAM(__FUNCTION__, ERROR, "Image data is wrong!");
                    nErrorCode += 2;
                }
                bFirstPacket = false;
                nBytesLeft = nBytesLeft - (nBytesAv - UDP_EXTRA_BYTES);
                nPos = nPos + static_cast<szt>(nBytesAv - UDP_EXTRA_BYTES);
            }

            if(nPos >= nFrameLength) {
                LOG_INFOS("One image is finished.");
                if(nReceivedData < nFrameLength)
                    nErrorCode++;

                break;
            }
        }
        LOG_INFOS(("Total Received Data:" + to_string(static_cast<long long>(nReceivedData)) + "Error code is:"
                   + to_string(static_cast<long long>(nErrorCode))));
        return nErrorCode;
    }

    int32 NetworkUDPImplementation::ReceiveData(char* ptrchData, szt nLengthMin, szt nLengthMax, szt& nTotalReceived)
    {
        // NOT YET IMPLEMENTED
        LOG_TRACE(__FUNCTION__);
        return -1;
    }

    int32 NetworkUDPImplementation::SendData(char* ptrchData, szt nLength)
    {
        LOG_TRACE(__FUNCTION__);
        /// not used
        /// currently udp protocol only used for receive image data from detector
        return -1;
    }

    int32 NetworkUDPImplementation::SendData(vector<uchar> vChData)
    {
        LOG_TRACE(__FUNCTION__);
        return -1;
    }

    void NetworkUDPImplementation::ClearDataInSocket()
    {
        LOG_TRACE(__FUNCTION__);

        char* ptrchPacket = new char[UDP_PACKET_SIZE_NORMAL];
        szt nBytesLength;
        int32 nCount = 0;

        while(ReceivePacket(ptrchPacket,nBytesLength) != -1)
        {
            nCount++;
        }

        LOG_INFOS("Get Extra packets:" + to_string(nCount));
        delete [] ptrchPacket;
    }
}
