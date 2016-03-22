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

#include "CheckEquality.h"

#include <algorithm>

namespace DetUnitTestNS
{

    bool CheckEquality::CheckEqualityBool(const bool bVal1, const bool bVal2)
    {
        return (bVal1==bVal2);
    }
    
    bool CheckEquality::CheckEqualityInt(const int nVal1,const int nVal2)
    {
        return (nVal1==nVal2);
    }
    
    bool CheckEquality::CheckEqualityDouble(const double dVal1, const double dVal2)
    {
        return (dVal1==dVal2);
    }
    
    bool CheckEquality::CheckEqualityString(const std::string strVal1, const std::string strVal2)
    {
        return (strVal1==strVal2);
    }

    bool CheckEquality::CheckEqualityVecChar(const std::vector<char> vchData1,const std::vector<char> vchData2)
    {
        return std::equal(vchData1.begin(),vchData1.end(),vchData2.begin());
    }
    
    
}

