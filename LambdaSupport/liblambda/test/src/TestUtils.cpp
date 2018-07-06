#include "TestUtils.h"

namespace TestNS
{
    void TimeMeasurement::Start()
    {
        m_start = std::chrono::steady_clock::now();
    }

    void TimeMeasurement::Stop()
    {
        m_stop = std::chrono::steady_clock::now();
    }

    uint64 TimeMeasurement::GetDifferenceInMicroSecond()
    {
        return (std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count());
    }
}
