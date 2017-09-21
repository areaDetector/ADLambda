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

#pragma once

#include "Globals.h"

namespace FSDetCoreNS
{
    /**
     * @brief string utils
     * This class provide string process method.
     */
    class StringUtils
    {
      public:
        /**
         * @brief string split
         * @param string original string
         * @param strDelimiter delimiter for splitting string
         * @param vStrArray returned string array
         */
        void StrSplit(const string strOriginal,
                      const string strDelimiter,
                      vector<string>& vStrArray);

        /**
         * @brief replace specific parts of one string
         * @param strSrc source string needs to be processed
         * @param strFind string part needs to be replaced
         * @param strReplace replacement of strFind
         */
        void FindAndReplace(string& strSrc,
                            string const& strFind,
                            string const& strReplace);

        void RemoveChar(string& strVal, char chDelimiter);

        /**
         * @brief Bitwise reverse a char, e.g. 00010010 -> 01001000
         * @param charSrc source char needs to be reversed
         * @return reversed char value
         */
        static char ReverseChar(char charSrc);
    };

    /**
     * @brief file utils
     * This class provide file related method
     */
    class FileUtils
    {
      public:
        /**
         * @brief get all file names under directory
         * @param strPath  path to read
         * @return file list
         */
        vector<string> GetFileList(string strPath);
    };

    /**
     * @brief provide data convert method
     */
    class DataConvert
    {
      public:
        void IntToUChar(int32 nVal, vector<uint8>& vUChar);
    };
}
