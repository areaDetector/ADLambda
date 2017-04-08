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

#ifndef __ZLIB_WRAPPER_H__
#define __ZLIB_WRAPPER_H__

#include <iostream>
#include <memory>
#include <vector>
#include <zlib.h>
#include <math.h>

///namespace 
namespace CompressionNS
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
     * @brief structure of z_stream
     */
    /* typedef struct z_stream_s */
    /* { */
    /*     z_const Bytef *next_in;     /\* next input byte *\/ */
    /*     uInt     avail_in;  /\* number of bytes available at next_in *\/ */
    /*     uLong    total_in;  /\* total number of input bytes read so far *\/ */

    /*     Bytef    *next_out; /\* next output byte should be put there *\/ */
    /*     uInt     avail_out; /\* remaining free space at next_out *\/ */
    /*     uLong    total_out; /\* total number of bytes output so far *\/ */

    /*     z_const char *msg;  /\* last error message, NULL if no error *\/ */
    /*     struct internal_state FAR *state; /\* not visible by applications *\/ */

    /*     alloc_func zalloc;  /\* used to allocate the internal state *\/ */
    /*     free_func  zfree;   /\* used to free the internal state *\/ */
    /*     voidpf     opaque;  /\* private data object passed to zalloc and zfree *\/ */

    /*     int     data_type;  /\* best guess about the data type: binary or text *\/ */
    /*     uLong   adler;      /\* adler32 value of the uncompressed data *\/ */
    /*     uLong   reserved;   /\* reserved for future use *\/ */
    /* } z_stream;
     */
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
         * @brief set next in buffer
         * @param next input byte
         */
        void SetNextIn(vector<unsigned char> vuchNextIn);

        /**
         * @brief set numbers of bytes available at next_in
         * @param number of bytes available at next_in
         */
        void SetNextInBytes(unsigned int unAvailableInBytes);

        /**
         * @brief set next output byte
         * @param net output byte
         */
        void SetNextOut(vector<unsigned char> vuchNextOut);

        /**
         * @brief set remaining free space at next_out
         * @param free space at next_out
         */
        void SetNextOutBytes(unsigned int unAvailableOutBytes);

        /**
         * @brief get available out
         * @return next out bytes
         */
        unsigned int GetNextOutBytes() const;

        /**
         * @brief get error message during compression
         * @return error message. if NO error, it is NUll
         */
        string GetErrorMsg();
        

        /**
         * @brief allocate internal state to be NULL
         */
        void ResetZAlloc();
        
        /**
         * @brief free the internal state
         */
        void ResetZFree();

        /**
         * @brief set data object passed to zalloc and zfree
         */
        void ResetOpaque();
        
        /**
         * @brief init deflate
         * @param compression level
         * @return error code 0:OK
         */
        int DeflateInit(int nLevel);
        /**
         * @brief compress data
         * @param 0 : Z_NO_FLUSH;4: Z_FINISH
         * @return error code 0:OK
         */
        int Deflate(int nFlush);

        /**
         * @brief deflate end
         * @return error code 0: OK
         */
        int DeflateEnd();

        /**
         * @brief init inflate
         * @reuturn error code 0:OK
         */
        int InflateInit();

        /**
         * @brief decompress data
         * @return error code  0:OK
         */
        int Inflate();

        /**
         * @brief inflate end
         * @return error code 0:OK
         */
        int InflateEnd();

        /**
         * @brief compression
         * @param source data
         * @param dst data
         * @param compression level
         * @param error code 0:OK
         */
        int Compress(vector<unsigned char>& vuchSrcData,vector<unsigned char>& vuchDstData, int nLevel);

        /**
         * @brief adjust deflate size
         * @param origin size
         * @return adjust size
         */
        int DeflateSizeAdjust(int nSize);
      private:
        z_stream m_stZS;
        
    };//end of class
    
}///end of namespace


#endif
