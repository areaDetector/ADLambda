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


#include <memory>
#include <vector>
#include <zlib.h>
#include <math.h>
#include "LambdaGlobals.h"

namespace DetLambdaNS
{
    using namespace std;

    /**
     * @brief enum of return values
     */
    /* #define Z_OK            0 */
    /* #define Z_STREAM_END    1 */
    /* #define Z_NEED_DICT     2 */
    /* #define Z_ERRNO        (-1) */
    /* #define Z_STREAM_ERROR (-2) */
    /* #define Z_DATA_ERROR   (-3) */
    /* #define Z_MEM_ERROR    (-4) */
    /* #define Z_BUF_ERROR    (-5) */
    /* #define Z_VERSION_ERROR (-6) */
    
    /**
     * @brief zlib wrapper
     */
    class ZlibWrapper
    {
      public:
        /**
         * @brief constructor
         */
        ZlibWrapper();

        /**
         * @brief destructor
         */
        ~ZlibWrapper();
        
        /**
         * @brief compression
         * @param source data
         * @param dst data
         * @param compression level
         * @param error code 0:OK
         */
        int32 Compress(vector<uchar>& vuchSrcData,vector<uchar>& vuchDstData, szt nLevel);

        /**
         * @brief adjust deflate size
         * @param origin size
         * @return adjust size
         */
        int32 DeflateSizeAdjust(int32 nSize);
      private:
        z_stream m_stZS;
        
    };
}
