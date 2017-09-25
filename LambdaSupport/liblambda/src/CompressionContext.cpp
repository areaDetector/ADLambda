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

#include "CompressionContext.h"
#include "Compression.h"

namespace DetLambdaNS
{
    CompressionContext::CompressionContext(unique_ptr<CompressionInterface> _uptrCompInterface)
        :m_uptrCompInterface(move(_uptrCompInterface))
    {}

    CompressionContext::~CompressionContext()
    {
        m_uptrCompInterface.reset();
    }
    
    bool CompressionContext::CompressData(vector<uchar>& vuchSrcData,
                                          vector<uchar>& vuchDstData,szt nLevel)
    {
        return m_uptrCompInterface->CompressData(vuchSrcData,vuchDstData,nLevel);
    }
    
    bool CompressionContext::DecompressData(vector<uchar>& vuchSrcData,
                                            vector<uchar>& vuchDstData)
    {
        return m_uptrCompInterface->DecompressData(vuchSrcData,vuchDstData);
    }
    
  
    string CompressionContext::GetErrorMessage()
    {
        return m_uptrCompInterface->GetErrorMessage();
    }
   
}
