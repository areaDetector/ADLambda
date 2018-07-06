#pragma once

#include "LambdaGlobals.h"
#include <gtest/gtest.h>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <stdio.h>
#include <stdarg.h>

namespace TestNS
{
    using namespace FSDetCoreNS;
    
    class TimeMeasurement
    {
    public:
        void Start();
        void Stop();
        uint64 GetDifferenceInMicroSecond();

    private:
        std::chrono::steady_clock::time_point m_start;
        std::chrono::steady_clock::time_point m_stop;
    };
    
}
