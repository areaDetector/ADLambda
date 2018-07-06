#pragma once

#include "LambdaConfigReader.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
namespace TestNS
{
    using namespace FSDetCoreNS;
    using namespace DetLambdaNS;
    
    using namespace std;

    using ::testing::SaveArg;
    using ::testing::AtLeast;
    using ::testing::Return;
    using ::testing::_;

    typedef unique_ptr<LambdaConfigReader> uptr_config;
    
    class ConfigReaderTest : public testing::Test
    {
    protected:
        virtual void SetUp();
        virtual void TearDown();

        uptr_config m_configreader;        
    };    
}
