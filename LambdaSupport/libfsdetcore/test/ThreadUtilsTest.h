#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Globals.h"
#include "ThreadUtils.h"

namespace TestNS
{
    using namespace FSDetCoreNS;
    using ::testing::AtLeast;
    using ::testing::Return;

    class ThreadUtilsTest : public testing::Test
    {
      protected:
        virtual void SetUp();
        virtual void TearDown();

        int32 m_threadnumbers;
        unique_ptr<ThreadPool> m_threadpool;
    };
}
