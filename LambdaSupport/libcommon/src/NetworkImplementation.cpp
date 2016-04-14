
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

#include "NetworkImplementation.h"

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
        
        int nOptVal =1;
        int nOptlen;
        
        ///initialize socket
        int nProtocol = GetProtocol(TCP);

        ///create socket
        m_nSockfd = socket(AF_INET,nProtocol,0);
        //setsockopt(m_nSockfd,SOL_SOCKET,SO_REUSEADDR,&nOptVal,sizeof(nOptVal));

        if(m_nSockfd < 0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot open socket!");

        ///try to get host via IP address
        m_stServer = gethostbyname(m_strIP.c_str());
        if(m_stServer == NULL)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"cannot find host!");
            exit(0);
        }

        ///set timeout for receiving data
        ///avoid recvfrom hanging
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        if(setsockopt(m_nSockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv))<0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot set socket timeout");

        bzero((char*) &m_stServAddr,sizeof(m_stServAddr));
        m_stServAddr.sin_family = AF_INET;
        bcopy((char*) m_stServer->h_addr, (char*) &m_stServAddr.sin_addr.s_addr, m_stServer->h_length);
        m_stServAddr.sin_port = htons(m_nPort);

        LOG_INFOS("TCP initializes successfully.");
    }
        
    NetworkTCPImplementation::~NetworkTCPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }
       
    int NetworkTCPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        int nRet = connect(m_nSockfd, (struct sockaddr*) &m_stServAddr,sizeof(m_stServAddr));
        if(nRet < 0)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"TCP cannot connect to host! Unable to connect to detector - please check that it is powered and correctly connected");
	    LOG_STREAM(__FUNCTION__,ERROR,m_strIP);
            return nRet;
        }
            
        LOG_INFOS("connected to host successfully via TCP protocol.");
        m_bLive = true;
        return nRet;
    }
        
    int NetworkTCPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        ///close socket
        close(m_nSockfd);
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

        int nBytesRead = 0;
        int nTotalReceivedPacket = 0;
        int nRet = -1;
        int nRemainByte = nLength;
        int nReadLen = 1500;
        char* ptrChRec = new char[nReadLen];
        int nTry = 0;
	int nMaxTries = 10000;

        while(nTotalReceivedPacket<nLength)
        {
            nRemainByte = nLength-nTotalReceivedPacket;
            if(nRemainByte<nReadLen)
                nReadLen = nRemainByte;
            //read one image
            if((nRet =::recv(m_nSockfd,ptrChRec,nReadLen,0))<0)
            {
                //cout<<"No Data Received:"<<nRet<<endl;
                nTry++;
                if(nTry>nMaxTries)
                {
                    delete ptrChRec;
                    return -1;
                }
            }
            else
            {
                nTry = 0;
                std::copy(ptrChRec,ptrChRec+nRet,ptrchData+nTotalReceivedPacket);
                nTotalReceivedPacket+=nRet;
		std::cout<<"nTotalRecived:"<<nTotalReceivedPacket<<"This packet:"<<nRet<<endl;
            }
        }
        delete ptrChRec;
        return 0;
    }

    int NetworkTCPImplementation::SendData(char* ptrchData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);

        int nRet = -1;
        nRet = send(m_nSockfd,ptrchData,nLength,0);
        if(nRet == -1)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"send data error!");
        }
        else
        {
            if(nRet == nLength)
                LOG_INFOS("send data successfully.");
        }
        
        return nRet;
    }
        
    int NetworkTCPImplementation::SendData(vector<unsigned char> vChData)
    {
        LOG_TRACE(__FUNCTION__);

        int nRet = -1;
        int nSize = vChData.size();
        char* ptrchData = reinterpret_cast<char*>(&vChData[0]);
        nRet = send(m_nSockfd,ptrchData,nSize,0);
        if(nRet == -1)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"send data error!");
        }
        else
        {
            if(nRet == nSize)
                LOG_INFOS("send data successfully.");
        }
        
        return nRet;
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
    
        int nOptVal =1;
        int nOptlen;
        ///initialize socket
        int nProtocol = GetProtocol(UDP);

        ///create socket
        m_nSockfd = socket(AF_INET,nProtocol,0);
        setsockopt(m_nSockfd,SOL_SOCKET,SO_REUSEADDR,&nOptVal,sizeof(nOptVal));
    
        if(m_nSockfd < 0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot open socket!");
        
        ///set timeout for receiving data
        ///avoid recvfrom hanging
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        if(setsockopt(m_nSockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv))<0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot set socket timeout");

        ///set socket buffer size
        int nRcvBuffLen;
        int nRcvLen = sizeof(nRcvBuffLen);
        if(getsockopt(m_nSockfd,SOL_SOCKET,SO_RCVBUF,(void*)&nRcvBuffLen,(socklen_t *)&nRcvLen)<0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot get the socket buffer size");
        //printf("the recevice buf len: %d\n", nRcvBuffLen);

        nRcvBuffLen *=5;
        nRcvLen = sizeof(nRcvBuffLen);
        
        if(setsockopt(m_nSockfd,SOL_SOCKET,SO_RCVBUF,(void*)&nRcvBuffLen,nRcvLen)<0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot set the socket buffer size");
        
        if( getsockopt(m_nSockfd,SOL_SOCKET,SO_RCVBUF,(void *)&nRcvBuffLen,(socklen_t *)&nRcvLen)<0)
            LOG_STREAM(__FUNCTION__,ERROR,"cannot get the socket buffer size1");
        
        ///try to get host via IP address
        m_stServer = gethostbyname(m_strIP.c_str());
        if(m_stServer == NULL)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"UDP cannot find host!");
            exit(0);
        }

        bzero((char*) &m_stServAddr,sizeof(m_stServAddr));
        m_stServAddr.sin_family = AF_INET;
        bcopy((char*) m_stServer->h_addr, (char*) &m_stServAddr.sin_addr.s_addr, m_stServer->h_length);
        m_stServAddr.sin_port = htons(m_nPort);
        
        LOG_INFOS("UDP initializes successfully.");
    }
        
    NetworkUDPImplementation::~NetworkUDPImplementation()
    {
        LOG_TRACE(__FUNCTION__);
    }
        
    int NetworkUDPImplementation::Connect()
    {
        LOG_TRACE(__FUNCTION__);

        int nRet = ::bind(m_nSockfd, (struct sockaddr*) &m_stServAddr,sizeof(m_stServAddr));
        if(nRet < 0)
        {    
            LOG_STREAM(__FUNCTION__,ERROR,"UDP bind failed!");
            return nRet;
        }

        LOG_INFOS("connected to host successfully via UDP protocol.");
        m_bLive = true;
        return nRet;
    }

    int NetworkUDPImplementation::Disconnect()
    {
        LOG_TRACE(__FUNCTION__);

        ///close socket
        close(m_nSockfd);
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
        int nAddrLen = sizeof(stAddr);
        int nRetVal = -1;

        do
        {
            char chTemp;
            nRetVal = ::recvfrom(m_nSockfd,&chTemp,1,MSG_PEEK,(struct sockaddr*)&stAddr,(socklen_t*)&nAddrLen);
        }
        while(nRetVal == -1 && errno == EINTR);

        bool bHasData = (nRetVal!=-1)||errno == EMSGSIZE;
        return bHasData;
    
    }

    int NetworkUDPImplementation::HasDataSize()
    {
        LOG_TRACE(__FUNCTION__);
        
        int nRetVal = -1;
        int nTempSize = 9600;
        char* chData = new char[nTempSize];
        while(true)
        {
            nRetVal = ::recv(m_nSockfd,chData,nTempSize,MSG_PEEK);
            if (nRetVal == -1 && errno == EINTR)
                continue;
            if(nRetVal!=nTempSize)
                break;
	    
            nTempSize*=2;
            delete chData;
            chData = new char[nTempSize];   

        }
        delete chData;
        return nRetVal;	
    }


    int NetworkUDPImplementation::ReceivePacket(char* ptrchData,int& nLength)
    {   
        int nBytesAv = 0;
        int nErrorCode = -1;
        int nRetVal = -1;
        struct sockaddr_in stAddr;
        int nAddrLen = sizeof(stAddr);
        if(HasData())
        {
            nBytesAv = HasDataSize();
            nRetVal = ::recvfrom(m_nSockfd, (char*)ptrchData, nBytesAv, 0,(struct sockaddr*)&stAddr,(socklen_t*)&nAddrLen);
            nLength = nRetVal;
            if(nBytesAv!=nRetVal)
                return 1;
            else 
                return 0;
        }
        return -1;//no data
    }
    
    int NetworkUDPImplementation::ReceiveData(char* ptrchData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);

        bool bFirstPacket = true;            
        int nErrorCode = 0;
        int nReceivedData = 0;
        int nRetVal = -1;
        int nFrameLength = nLength;
        int nBytesLeft = nLength;
        int nPos = 0;
        int nBytesAv = 0;   
        int nTotalBytesReceived = 0;
        int nCounter = 0;  
    
        struct sockaddr_in stAddr;
        int nAddrLen = sizeof(stAddr);
    
        int nPacketLength = UDP_PACKET_SIZE_NORMAL;
        char* chDataRec = new char[nPacketLength];

	//cout<<"Receive Data started"<<endl;
	
        while(1) // We break when the image is finished
        {
	  
	    while(!HasData())
	    {
	        usleep(50); // Don't poll excessively if no data available
	    }

            nBytesAv = HasDataSize();  // Check packet size
            if(nBytesAv > nBytesLeft+UDP_EXTRA_BYTES)
            {
                //data already out of bounds;
                nErrorCode+=4;
                LOG_STREAM(__FUNCTION__,ERROR,"Image data is out of bound");
                LOG_INFOS(("Data out of bound:"+to_string(static_cast<long long>(nBytesAv))+"Data remain:"+to_string(static_cast<long long>(nBytesLeft))));
                break;
            }

            nRetVal = ::recvfrom(m_nSockfd, (char*)chDataRec, nBytesAv, 0,(struct sockaddr*)&stAddr,(socklen_t*)&nAddrLen);
            nCounter++;
	    
            if(!bFirstPacket)
            {
                //copy data to destination buffer, needs to skip the UDP_EXTRA_BYTES
                if(nRetVal>0)
                {
                    std::copy(chDataRec+UDP_EXTRA_BYTES,chDataRec+nRetVal,ptrchData+nPos);
                    nReceivedData += nRetVal;
                }
                nBytesLeft = nBytesLeft-(nBytesAv-UDP_EXTRA_BYTES);
                nPos = nPos + nBytesAv-UDP_EXTRA_BYTES;
            }
            else
            {
                if(nRetVal >0)
                {
                    std::copy(chDataRec,chDataRec+nRetVal,ptrchData);
                    nReceivedData += nRetVal;
                }

                //first byte of the image should be 0xa0
                unsigned char uchByte = ptrchData[UDP_EXTRA_BYTES];
                if(uchByte!=0xa0)
                {	
                    LOG_STREAM(__FUNCTION__,ERROR,"Image data is wrong!");
                    nErrorCode += 2;    
                }     
                bFirstPacket = false;
                nBytesLeft = nBytesLeft-nBytesAv;
                nPos = nPos +  nBytesAv;
            } 
	
            if(nPos >= nFrameLength)
            {
                LOG_INFOS("One image is finished.");
                if(nReceivedData<nFrameLength)
                    nErrorCode++;
	
                break;
            }	
        }
        LOG_INFOS(("Total Received Data:"+to_string(static_cast<long long>(nReceivedData))+"Error code is:"+to_string(static_cast<long long>(nErrorCode))));
        delete chDataRec;
        return nErrorCode;        
    }

    int NetworkUDPImplementation::SendData(char* ptrchData,int nLength)
    {
        LOG_TRACE(__FUNCTION__);
        /// not used
        /// currently udp protocol only used for receive image data from detector
    }        

    int NetworkUDPImplementation::SendData(vector<unsigned char> vChData)
    {
        LOG_TRACE(__FUNCTION__);
    }
    
   
}///end of namespace DetCommonNS
