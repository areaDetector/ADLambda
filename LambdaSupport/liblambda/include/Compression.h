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

#ifndef __COMPRESSION_H__
#define __COMPRESSION_H__

#include <iostream>
#include <memory>
#include <vector>

///namespace 
namespace CompressionNS
{   
    using namespace std;
    
    class ZlibWrapper;
    
    /**
     * @brief interface of compression class
     */
    class CompressionInterface
    {
      public:
        /**
         * @brief compress data
         * @param src data
         * @param compressed data
         * @param compression level,between 0-9. Default value is 2
         * @return true: OK; false: error during compression
         */
        virtual bool CompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData,int nLevel = 2) = 0;

        /**
         * @brief decompress data
         * @param src data
         * @param compressed data
         * @return true: OK; false: error during decompression
         */
        virtual bool DecompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData) = 0;

        /**
         * @brief get error message
         * @return error message
         */
        virtual string GetErrorMessage();

      protected:
        string m_strErrMsg;
        
    };///end of class

    /**
     * @brief compression with zlib
     */
    class CompressionZlib : public CompressionInterface
    {
        
      public:
        /**
         * @brief constructor
         */
        CompressionZlib();
        
        /**
         * @brief destructor
         */
        ~CompressionZlib();
        
        bool CompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData,int nLevel = 2);
        bool DecompressData(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData);
        
      private:
        shared_ptr<ZlibWrapper> m_sptrZlibWrapper;
        
        
    };///end of class
    
    
    
}///end of namespace


#endif
