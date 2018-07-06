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

#include "LambdaGlobals.h"

#ifdef ENABLEHWCOMPRESSION
#include "ahagz_api.h"
#endif

namespace DetLambdaNS
{
    class ZlibWrapper;

    /**
     * @brief interface of compression class
     */
    class CompressionInterface
    {
    public:
        virtual ~CompressionInterface(){}
        /**
         * @brief compress data
         * @param src data
         * @param compressed data
         * @param compression level,between 0-9. Default value is 2
         * @return true: OK; false: error during compression
         */
        virtual bool CompressData(vector<uchar>& vuchSrcData,
                                  vector<uchar>& vuchDstData,
                                  szt nLevel = 2) = 0;

        /**
         * @brief decompress data
         * @param src data
         * @param compressed data
         * @return true: OK; false: error during decompression
         */
        virtual bool DecompressData(vector<uchar>& vuchSrcData,
                                    vector<uchar>& vuchDstData) = 0;

        /**
         * @brief get error message
         * @return error message
         */
        virtual string GetErrorMessage();

      protected:
        string m_strErrMsg;

    };

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

        bool CompressData(vector<uchar>& vuchSrcData,
                          vector<uchar>& vuchDstData,
                          szt nLevel = 2);
        bool DecompressData(vector<uchar>& vuchSrcData,
                            vector<uchar>& vuchDstData);

      private:
        shared_ptr<ZlibWrapper> m_sptrZlibWrapper;
    };

    #ifdef ENABLEHWCOMPRESSION
    //////////////////////////////////////////////////
    /// CompressionHWAHA
    //////////////////////////////////////////////////
    /**
     * @brief compression with aha hardware aha374/aha378
     */
    class CompressionHWAHA : public CompressionInterface
    {
    public:
        /**
         * @brief constructor
         */
        CompressionHWAHA(int32 nChunkIn,int32 nChunkOut,uint8 unCtrl,int32 nPostCode);

        /**
         * @brief destructor
         */
        ~CompressionHWAHA();

        bool CompressData(vector<uchar>& vuchSrcData,
                          vector<uchar>& vuchDstData,
                          szt nLevel = 2);
        bool DecompressData(vector<uchar>& vuchSrcData,
                            vector<uchar>& vuchDstData);

    private:
        int32 Open();
        int32 Close();
        void AddInputBuffer();
        void AddOutputBuffer();

    private:
        int32 m_nBoard,m_nChunkIn,m_nChunkOut,m_nPostCode,
            m_nDataLength,m_nProcessed,m_nTotalSize;

        aha_stream_t m_as;
        uint8 m_unType;
        uint8 m_unCtrl;

        vector<uchar*> m_vOutBuffer;
        uchar* m_ptrDataSrc;
    };
    #endif
}
