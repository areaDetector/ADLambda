#pragma once

#include <fsdetector/core/NetworkInterface.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace FSDetCoreNS
{
    class MockNetwork : public NetworkInterface
    {
      public:
        virtual ~MockNetwork(){}
        MOCK_METHOD0(Connect,int32());
        MOCK_METHOD0(Disconnect,int32());
        MOCK_METHOD0(Live,bool());
        MOCK_METHOD0(ClearDataInSocket,void());
        MOCK_METHOD1(SendData,int32(vector<uchar>));
        MOCK_METHOD2(SendData,int32(char*,szt));
        MOCK_METHOD2(ReceivePacket,int32(char*,szt&));
        MOCK_METHOD2(ReceiveData,int32(char*,szt));
        MOCK_METHOD4(ReceiveData,int32(char*,szt,szt,szt&));
    };
}
