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


#include "ZlibWrapper.h"

namespace DetLambdaNS
{
    ZlibWrapper::ZlibWrapper()
        :m_stZS()
    {
    }

    ZlibWrapper::~ZlibWrapper()
    {
    }

    int32 ZlibWrapper::Compress(vector<uchar>& vuchSrcData,
                              vector<uchar>& vuchDstData, szt nLevel)
    {

        int32 nSrcSize = vuchSrcData.size();
        int32 nDstSize = DeflateSizeAdjust(nSrcSize);
        uLongf ulfDstSize = (uLongf)nDstSize;
        vuchDstData.resize(nDstSize);

        uchar* uptrSrcData = reinterpret_cast<uchar*>(&vuchSrcData[0]);
        uchar* uptrDstData = reinterpret_cast<uchar*>(&vuchDstData[0]);

        int32 nRetVal = compress2(uptrDstData,&(ulfDstSize),uptrSrcData,nSrcSize,nLevel);
        nDstSize = ulfDstSize;

        vuchDstData.resize(nDstSize);
        return nRetVal;
    }

    int32 ZlibWrapper::DeflateSizeAdjust(int32 nSize)
    {
        return (ceil(((double)(nSize))*1.001)+12);
    }
}
