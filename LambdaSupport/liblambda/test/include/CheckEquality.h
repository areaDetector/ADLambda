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

#ifndef __CHECK_EQUALITY_H__
#define __CHECK_EQUALITY_H__

#include <cppunit/extensions/HelperMacros.h>
namespace DetUnitTestNS
{
    class CheckEquality
    {
      public:
        bool CheckEqualityBool(const bool bVal1, const bool bVal2);
        bool CheckEqualityInt(const int nVal1,const int nVal2);
        bool CheckEqualityDouble(const double dVal1, const double dVal2);
        bool CheckEqualityString(const std::string strVal1, const std::string strVal2);
        bool CheckEqualityVecChar(const std::vector<char> vchData1,const std::vector<char> vchData2);
    };
}

#endif
