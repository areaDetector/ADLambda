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

#ifndef __COMPRESSION_CONTEXT_H__
#define __COMPRESSION_CONTEXT_H__

#include <iostream>
#include <memory>
#include <vector>

///namespace 
namespace CompressionNS
{
    using namespace std;
    
    class CompressionInterface;
    

    /**
     * @brief compression strategy choose
     */
    class CompressionContext
    {
      public:
        /**
         * @brief constructor
         * @param compression interface object
         */
        CompressionContext(unique_ptr<CompressionInterface> _uptrCompInterface);

        /**
         * @brief destructor
         */
        ~CompressionContext();

        /**
         * @brief compress data
         * @param src data
         * @param compressed data
         * @param compression level,between 0-9. Default value is 2
         * @return true: OK; false: error during compression
         */
        bool CompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData,int nLevel = 2);
         /**
         * @brief decompress data
         * @param src data
         * @param compressed data
         * @return true: OK; false: error during decompression
         */
        bool DecompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData);

        /**
         * @brief get error message
         * @return error message
         */
        string GetErrorMessage();
        
      private:
        unique_ptr<CompressionInterface> m_uptrCompInterface;
    };
    
    
}///end of namespace


#endif
