#include "ThreadUtilsTest.h"

namespace TestNS
{
    void ThreadUtilsTest::SetUp()
    {
        m_threadnumbers = 10;
    }

    void ThreadUtilsTest::TearDown()
    {
        
    }

    TEST_F(ThreadUtilsTest,TestThreadPoolInit)
    {
        m_threadpool = unique_ptr<ThreadPool>(new ThreadPool(m_threadnumbers));
        EXPECT_EQ(m_threadpool->GetAvailableThreads(),m_threadnumbers);
        m_threadpool.reset(nullptr);
    }
}
