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

#ifndef __IMAGE_DECODER_H__
#define __IMAGE_DECODER_H__

#include "LambdaGlobals.h"
#include <stdalign.h> 

///namespace DetCommonNS
namespace DetCommonNS
{
    /**
     * @brief class ImageDecoder
     */
    class ImageDecoder
    {

      public:
        /**
         * @brief constructor
         * @param _vCurrentChips current used chips
         */
        ImageDecoder(vector<short> _vCurrentChips);

        /**
         * @brief destructor
         */
        ~ImageDecoder();

        /**
         * @brief set raw image
         * @param ptrchRawImg raw img data
         */
        void SetRawImage(char* ptrchRawImg);
            
        /**
         * @brief decode image
         * @return pointer to decoded image (within decoder)
         */
        short* RunDecodingImg();

       /**
         * @brief Check the dimensions of a decoded image (determined by chip layout - doesn't include any subsequent distortion correction)
         * @param Pass by reference - returns x and y size of decoded image in pixels
         */
	void GetDecodedImageSize(int& nX, int& nY);
        
      private:
        /**
         * @brief reorder raw data
         * @parblock
         * The idea is going through each element of  block in raw data,
         * copy it into reordered data.
         * see below
         * e.g. for 6 chips raw data
         * ---------------------------------------------------
         * |                block 1   | block 2   |...       |
         * | |image header|1|2|3|4|5|6|1|2|3|4|5|6|...       | 
         * --------------------------------------------------- 
         * reodered data
         * ---------------------------------------
         * |             from block 2            |
         * |                 |                   |
         * | |image header|1|1|...|2|2|...       |
         * |               |                     |
         * |         from block 1                |
         * ---------------------------------------
         *The algorithm of this method is:
         * 1. Going through every 32 bytes element in raw data
         * 2. Caculate the related position in reordered data
         * 3. copy the element in to reordered data
         * @endparblock
         */
        void ReorderRawData();

        /**
         * @brief reshuffle data
         * @parblock
         * The idea is going through each byte in m_ptrchReorderedData.
         * For each byte, put each bit into 8 separate consecutive pixel
         * e.g. Reorderd data
         * -----------------------------
         * | |Chip1|Chip2|...          |
         * |   |                       |
         * | 256*256*12 Byte           |
         * -----------------------------
         * First Byte for chip 1
         * -----------------------------
         * |   |1|2|3|4|5|6|7|8|       |
         * -----------------------------
         * Reshuffled First byte
         * ---------------------------------
         * |   (|1|...)(|2|...)...(|8|...) |
         * |       |                       |
         * |   1st bit of pixel 1          |
         * ---------------------------------
         * The algorithm is:
         * 1. Start from start pos 0,go through each byte in reordered data
         * 2. For each byte, do before all bytes are shuffled:
         *    2.1. Get 8 bits of the byte
         *    2.2. From start pos Each bit is put into each pixel as specific bit.
         *         e.g. For first byte from start pos 0, it can fill every first bit of pixel 1-8
         *    2.3. Go on reading by byte, until every first bit of pixels of row 0 (256pixels) is filled
         *    2.4. Go back to start pos metioned in 2.2. Fill the every second bit of all pixels of row 0
         *    2.5. depth of the pixel is 12bit, so once all the pixel bits are filled in the row 0(256*256*12/8 Byte). Increase row number to row 1, start pos becomes row*pixels_in_row(256).Go back to step 2.1
         *    @endparblock
         */
        void ReshuffleBitsInRawData();

        void ReshuffleBitsInRawDataFast();	// Test code for faster reshuffling


        /**
         * @brief Build images
         * @parblock
         * In the detector, the chip from 7-12 should be rotated
         * -------------------------
         * | 1 | 2 | 3 | 4 | 5 | 6 |
         * -------------------------
         * |12 |11 |10 | 9 | 8 | 7 |
         * -------------------------
         * @endparblock
         */
        void BuildFullImage();
            
        void BuildFullImageFast();


        /**
         * @brief extract pixel values by using lookup table
         */
        void ExtractPixelVal();
            
        /**
         * @brief calculate position of chips, chip no from 6-12 needs to be rotated
         */
        void CalculateChipPos();            

        /**
         * @brief generate 12bit lookup table. Take from old version library
         * @return lookup table
         */
        vector<unsigned short> Generate12BitTable();

        /**
         * @brief generate lookup table for doing fast bit reshuffling.
         * @return lookup table
         */
        vector<unsigned long long> GenerateReshuffleLUT();
            
      private:
        char* m_ptrchRawImg;
        unsigned char* m_ptrchReorderedData;
        unsigned short* m_ptrshReshuffledData;
        short* m_ptrshFullImg;
        unsigned short* m_ptrshDecodedImg;

	int m_nChipHeaderLength;
        int m_nTotalImgLength;
        int m_nImgLength;
           
	    //without header
        int m_nImgDataLength;
        int m_nModuleSizeX;
        int m_nModuleSizeY;
        int m_nChipNumbers;

        vector<short> m_vCurrentUsedChips;
        vector<short> m_vChipRotated;
        vector<short> m_vXstartPos;
        vector<short> m_vYstartPos;
        vector<short> m_vFullImg;
        vector<unsigned short> m_vLookuptable;

        vector<unsigned long long> m_vReshuffleLUT;

    };///end of class ImageDecoder
}///end of namespace DetCommonNS

#endif 
