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

#include "ImageDecoder.h"

namespace DetLambdaNS
{
    ImageDecoder::ImageDecoder(vector<int16> _vCurrentChips)
    {
        LOG_TRACE(__FUNCTION__);
            
        m_vCurrentUsedChips = _vCurrentChips;
        m_nChipNumbers = m_vCurrentUsedChips.size();
        m_nChipHeaderLength = CHIP_HEADER_SIZE*m_nChipNumbers;
        m_vChipRotated.resize(STANDARD_CHIP_NUMBERS);
        m_vXstartPos.resize(STANDARD_CHIP_NUMBERS);
        m_vYstartPos.resize(STANDARD_CHIP_NUMBERS);
        CalculateChipPos();
        
        m_nImgDataLength = m_nChipNumbers*BYTES_IN_CHIP;
        m_nTotalImgLength = m_nChipHeaderLength + m_nImgDataLength;
        m_nImgLength = m_nChipNumbers*PIXELS_IN_CHIP;

        //These variables don't include image header
        m_ptrchReorderedData = new uchar[m_nImgDataLength];

        // In theory, new in C++ standard should automatically give at least 8-byte alignment.
        m_ptrshReshuffledData = new uint16[m_nImgLength];
        m_ptrshFullImg = new int16[m_nImgLength];
            
        //generate lookup table
        m_vLookuptable = Generate12BitTable(); 
        m_vReshuffleLUT = GenerateReshuffleLUT();   
    }

    ImageDecoder::~ImageDecoder()
    {
        LOG_TRACE(__FUNCTION__);

        delete [] m_ptrchReorderedData;
        delete [] m_ptrshReshuffledData;
        delete [] m_ptrshFullImg;
        m_vCurrentUsedChips.clear();
        m_vChipRotated.clear();
        m_vXstartPos.clear();
        m_vYstartPos.clear();
        m_vFullImg.clear();
        m_vLookuptable.clear();
    }

    void ImageDecoder::SetRawImage(char* ptrchRawImg)
    {
        LOG_TRACE(__FUNCTION__);
        
        m_ptrchRawImg = ptrchRawImg;
    }

    int16* ImageDecoder::RunDecodingImg()
    {
        LOG_TRACE(__FUNCTION__);
        
        //refill temporary image data with 0
        memset(m_ptrchReorderedData,0,m_nImgDataLength);
        
        // We have short values - need to *2 to get no of bytes in image
        memset(m_ptrshReshuffledData,0,(m_nImgLength*2)); 
        memset(m_ptrshFullImg,0,(m_nImgLength*2));
            
        ReorderRawData();
        ReshuffleBitsInRawDataFast();
        BuildFullImageFast();
        ExtractPixelVal();

        return m_ptrshFullImg;                 
    }
        
    void ImageDecoder::ReorderRawData()
    {
        LOG_TRACE(__FUNCTION__);

        //Here, the raw data stream is reordered so that all the data from
        //each chip is gathered together
        //first 32XNchips Bytes are headers for image
        //memcpy(m_ptrchReorderedData;m_ptrchRawImg;m_nChipHeaderLength);
        int32 nChipNo = 1;
        int32 nChipDataParts = 0;
        //don't need to process image header part
        for(int32 i=m_nChipHeaderLength;i<m_nTotalImgLength;i+=BLOCK_SIZE_IN_BYTES)
        {
            //calculate destination positions
            int32 nPos =(nChipNo-1)*BYTES_IN_CHIP //current chip No.
                +nChipDataParts*BLOCK_SIZE_IN_BYTES; //current parts for the specific chip
                
            memcpy(m_ptrchReorderedData+nPos,m_ptrchRawImg+i,BLOCK_SIZE_IN_BYTES);  

            if(nChipNo == m_nChipNumbers)
            {
                nChipDataParts++;
                nChipNo = 1;
            }
            else
                nChipNo++;
        }        
    }

    void ImageDecoder::ReshuffleBitsInRawData()
    {
        LOG_TRACE(__FUNCTION__);
        
        //each pixel has 12 bits
        int32 nBytesInRow = CHIP_SIZE * 12/8;

        //value from 0 - 255
        int32 nRow = 0;
        int32 nPos = 0;
        
        uchar chTemp;
        uchar chTempByte;
            
        //go through all bytes
        for(int32 i=0;i<m_nImgDataLength;i++)
        {
                
            chTempByte = m_ptrchReorderedData[i];
                
            for(int32 nBits=7;nBits>=0;nBits--)
            {
                chTemp = chTempByte;
                    
                chTemp >>= nBits;
                chTemp &= (0x1);

                m_ptrshReshuffledData[nPos] <<= 1;
                m_ptrshReshuffledData[nPos] |= chTemp;
                nPos++;
            }

            //check if one row is already finished
            if((i+1)%nBytesInRow==0)
                nRow++;
            
            //for each 32, the nPos needs to be reset
            if((i+1)%32==0)
                nPos = nRow*CHIP_SIZE;
        }         
    }
    
    void ImageDecoder::BuildFullImage()
    {
        LOG_TRACE(__FUNCTION__);
            
        int32 nChipNo = 1;
        int32 nStartPos = m_vXstartPos[m_vCurrentUsedChips[nChipNo-1]-1]
            +  m_vYstartPos[m_vCurrentUsedChips[nChipNo-1]-1]*m_nModuleSizeX;
            
        int32 nRow = 0;
        int32 nOffset = 0;
        int32 nPos = nStartPos+nRow*m_nModuleSizeX;
        int32 nRotatedChipStartPos = 0;
            
            
        for(int32 i=0;i<m_nImgLength;i++)
        {    
            if(m_vChipRotated[m_vCurrentUsedChips[nChipNo-1]-1])    
            {
                m_ptrshFullImg[nPos+nOffset] = m_ptrshReshuffledData[nRotatedChipStartPos--];
            }
            else
                m_ptrshFullImg[nPos+nOffset] = m_ptrshReshuffledData[i];
                
            nOffset++;
                
            //finish one row
            if(nOffset == CHIP_SIZE && i!=m_nImgLength-1)
            {    
                nOffset = 0;
                nRow++;
                    
                //already finish one chip
                if(nRow == CHIP_SIZE)
                {
                    nRow = 0;
                    nChipNo++;
                    nStartPos = m_vXstartPos[m_vCurrentUsedChips[nChipNo-1]-1]
                        +  m_vYstartPos[m_vCurrentUsedChips[nChipNo-1]-1]*m_nModuleSizeX;
                        
                    if(m_vChipRotated[m_vCurrentUsedChips[nChipNo-1]-1])
                        nRotatedChipStartPos = i+PIXELS_IN_CHIP;
                }                      

                nPos = nStartPos + nRow * m_nModuleSizeX;
            }
        }
    }



    void ImageDecoder::BuildFullImageFast()
    {
        LOG_TRACE(__FUNCTION__);

        int32 nChipsPresent = m_vCurrentUsedChips.size();
        int32 i=0;
        int32 outPos=0;

        for(int32 nChip=0; nChip<nChipsPresent;nChip++)
        {
            // Find index of start of chip in output array
            int32 nStartPos = m_vXstartPos[m_vCurrentUsedChips[nChip]-1]
                +  m_vYstartPos[m_vCurrentUsedChips[nChip]-1]*m_nModuleSizeX;
 
            // Behaviour depends on whether chip is rotated or not 
            if(m_vChipRotated[m_vCurrentUsedChips[nChip]-1])
            {
                // Need to walk through input data backwards!
                i+=PIXELS_IN_CHIP;  // Skip "i" forwards by 1 chip
                int32 irot=i;	        
                // Loop over rows in chip
                for(int32 nRow=0; nRow<CHIP_SIZE; nRow++)
                {
                    // Start position of each row
                    outPos = nStartPos + nRow * m_nModuleSizeX;
                    for(int32 nCol=0; nCol<CHIP_SIZE; nCol++)
                    {
                         // When working backwards using irot, pre-decrement to get right result
                        m_ptrshFullImg[outPos++] = m_ptrshReshuffledData[--irot];
                    }
                }
            }
            else
            {
                // Loop over rows in chip
                for(int32 nRow=0; nRow<CHIP_SIZE; nRow++)
                {
                    // Start position of each row
                    outPos = nStartPos + nRow * m_nModuleSizeX;
                    for(int32 nCol=0; nCol<CHIP_SIZE; nCol++)
                    {
                        m_ptrshFullImg[outPos++] = m_ptrshReshuffledData[i++];
                    }
                }
            }


        }

    }

        
    void ImageDecoder::CalculateChipPos()
    {
        LOG_TRACE(__FUNCTION__);
         
        int32 nMinXStartPos = INT_MAX;
        int32 nMaxXStartPos = INT_MIN;
        int32 nMinYStartPos = INT_MAX;
        int32 nMaxYStartPos = INT_MIN;
        int32 nCurrentXStartPos = 0;
        int32 nCurrentYStartPos = 0;
        bool bCurrentChipRotated = 0;
            
        for(int32 i=0;i<m_nChipNumbers;i++)
        {
            switch (m_vCurrentUsedChips[i])
            {
                case 1:
                    nCurrentXStartPos = 0;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;
                case 2:
                    nCurrentXStartPos = CHIP_SIZE;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;

                case 3:
                    nCurrentXStartPos = 2*CHIP_SIZE;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;

                case 4:
                    nCurrentXStartPos = 3*CHIP_SIZE;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;

                case 5:
                    nCurrentXStartPos = 4*CHIP_SIZE;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;

                case 6:
                    nCurrentXStartPos = 5*CHIP_SIZE;
                    nCurrentYStartPos = 0;
                    bCurrentChipRotated = 0;
                    break;
                    // AFTER CHIP 6, WE TAKE NEW ROW, GO IN OPPOSITE DIRECTION, AND ROTATE
                case 7:
                    nCurrentXStartPos = 5*CHIP_SIZE;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                case 8:
                    nCurrentXStartPos = 4*CHIP_SIZE;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                case 9:
                    nCurrentXStartPos = 3*CHIP_SIZE;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                case 10:
                    nCurrentXStartPos = 2*CHIP_SIZE;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                case 11:
                    nCurrentXStartPos = 1*CHIP_SIZE;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                case 12:
                    nCurrentXStartPos = 0;
                    nCurrentYStartPos = CHIP_SIZE;
                    bCurrentChipRotated = 1;
                    break;

                default: // If we have a strangely-numbered chip, we're screwed
                    break;
                        
            }
            
            //decide the maximum and minimum x,y start pos for chips
            nMinXStartPos
                = (nCurrentXStartPos < nMinXStartPos ? nCurrentXStartPos : nMinXStartPos);
            nMaxXStartPos
                = (nCurrentXStartPos > nMaxXStartPos ? nCurrentXStartPos : nMaxXStartPos);
            nMinYStartPos
                = (nCurrentYStartPos < nMinYStartPos ? nCurrentYStartPos : nMinYStartPos);
            nMaxYStartPos
                = (nCurrentYStartPos > nMaxYStartPos ? nCurrentYStartPos : nMaxYStartPos);
                
            m_vXstartPos[m_vCurrentUsedChips[i]-1] = nCurrentXStartPos;
            m_vYstartPos[m_vCurrentUsedChips[i]-1] = nCurrentYStartPos;
            m_vChipRotated[m_vCurrentUsedChips[i]-1] = bCurrentChipRotated;            
        }
        
        nMaxXStartPos -= nMinXStartPos;
        nMaxYStartPos -= nMinYStartPos;

        for(int32 i=0;i<m_nChipNumbers;i++)
        {
            m_vXstartPos[m_vCurrentUsedChips[i]-1] -= nMinXStartPos;
            m_vYstartPos[m_vCurrentUsedChips[i]-1] -= nMinYStartPos;
        }
        
        m_nModuleSizeX = nMaxXStartPos + CHIP_SIZE;
        m_nModuleSizeY = nMaxYStartPos + CHIP_SIZE;
    }
        

    void ImageDecoder::GetDecodedImageSize(int32& nX, int32& nY)
    {
        nX = m_nModuleSizeX;
        nY = m_nModuleSizeY;
    }


    void ImageDecoder::ExtractPixelVal()
    {
        LOG_TRACE(__FUNCTION__);
        uint16 shVal;
            
        for(int32 i=0;i<m_nImgLength;i++)
        {
            shVal = m_ptrshFullImg[i];
                
            //only keep 12 lower bits
            shVal &=4095;
            m_ptrshFullImg[i] = m_vLookuptable[shVal];
        }     
    }
        
    vector<uint16> ImageDecoder::Generate12BitTable()
    {
        LOG_TRACE(__FUNCTION__);
        
        //This function takes a 12bit value and returns the decimal code
        uint LFSR=0;//init LFSR
        uint b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,newb0;
        vector<uint16> vLFSR12bits(4096,0);
        for (int i=0;i<4096;i++)
        {
            // MODIFIED LUT - if we have pseudo-random counter output X, and true value Y, then LFSR12bits[X] = Y
            vLFSR12bits[LFSR]=i;
            //LFSR12bits[i]=LFSR;
            //Next code=(!b0 & !b1 & !b2 & !b3 & !b4& !b5& !b6 & !b7 & !b8 & !b9 & !b10) ^ b0 ^ b3 ^ b5 ^ b11
            //calculate next code
            b0=LFSR & 1;
            b1=(LFSR>>1) & 1;
            b2=(LFSR>>2) & 1;
            b3=(LFSR>>3) & 1;
            b4=(LFSR>>4) & 1;
            b5=(LFSR>>5) & 1;
            b6=(LFSR>>6) & 1;
            b7=(LFSR>>7) & 1;
            b8=(LFSR>>8) & 1;
            b9=(LFSR>>9) & 1;
            b10=(LFSR>>10) & 1;
            b11=(LFSR>>11) & 1;
            newb0=(!(b0|b1|b2|b3|b4|b5|b6|b7|b8|b9|b10))^b0^b3^b5^b11;
            //Shift
            b11=b10;
            b10=b9;
            b9=b8;
            b8=b7;
            b7=b6;
            b6=b5;
            b5=b4;
            b4=b3;
            b3=b2;
            b2=b1;
            b1=b0;
            b0=newb0;
            //Recalculate LFSR
            LFSR=b0|(b1<<1)|(b2<<2)|(b3<<3)|(b4<<4)|(b5<<5)|(b6<<6)|(b7<<7)|(b8<<8)|(b9<<9)|(b10<<10)|(b11<<11);
        }
        return vLFSR12bits;   
    }


    // Preliminary work on high-speed image decoding
    // Key goals; use 64-bit operations whenever possible, reduce unnecessary copying

    // Here, we generate 64-bit LUT that allows us to take a nibble of data from the
    // raw data stream and stick it into the short output stream directly
    // The key idea is that each bit in the raw data stream needs to be inserted
    // into successive elements in a short array. 
    // This is currently done 1 bit at a time, but it should be possible to take
    // 4 bits and look up a 64-bit word that can be inserted directly into the short array
    // For example, 0111 would give a lookup value of
    // 00000000000000000 0000000000000001 0000000000000001 0000000000000001
    // Note that the routine constructs this LUT by creating
    // short arrays and casting to long long. This ensures consistency regardless of endian format.

    vector<uint64> ImageDecoder::GenerateReshuffleLUT()
    {
        LOG_TRACE(__FUNCTION__);
        
        // This function takes a 4 bit value and returns a 64-bit word suitable for reshuffling
        // Will use a short array to convert to 64-bit value

        //uint LFSR=0;//init LFSR
        int32 tablesize = 16;
        vector<uint64> vReshuffleLUT(tablesize,0);
        uint16 shEntry[4];
        uint64* ptrlEntry; // Used to convert short array into long value!
        ptrlEntry = reinterpret_cast<uint64*>(&shEntry[0]);

        //std::cout << "Size of unsigned long long is: " << sizeof(unsigned long long) << "\n";

        //uint64 summationvalue;

        for (int32 i=0;i<tablesize;i++)
        {
            // Work over nibble - will just code directly
            shEntry[0] = (i>>3) & 1;
            shEntry[1] = (i>>2) & 1;
            shEntry[2] = (i>>1) & 1;
            shEntry[3] = i & 1;
            vReshuffleLUT[i] = *ptrlEntry; // Dereference should stick int array into long value
            //std::cout << "Lookup table entry " << i
            //<< " is long value " << vReshuffleLUT[i] << "\n";
        }

        //delete [] shEntry;
        return vReshuffleLUT;   
    }


    // High speed reshuffling routine possibly requires memory alignment of input and output arrays
    // For first implementation, can keep rest of routine as before and only redo bit reshuffling
    // Strategy here is to use 64-bit words during reshuffling rather than working bit-by-bit
    // This requires working through the short output array (2 bytes) using an 8-byte pointer
    // Our chip always has 256 pixel rows. This means 256 integer outputs in row, i.e. 512 bytes
    // in output stream, i.e. 64 long words or 32 pairs.
    // Given this, can mostly work using our 32-step reset as before

    void ImageDecoder::ReshuffleBitsInRawDataFast()
    {
        LOG_TRACE(__FUNCTION__);
        
        //each pixel has 12 bits
        int32 nBytesInInputRow = CHIP_SIZE * 12/8;
        
        // A row of 256 short output values corresponds to 256*2/8 longs.
        int32 nLongsInOutputRow = CHIP_SIZE*2/8; 

        //value from 0 - 255
        int32 nRow = 0;
        int32 nPos = 0; // This now keeps track of long pointer position
        
        //uchar chTemp;
        uchar chTempByte;
        uint64 lLookupVal;
        uint64* ptrlongReshuffledData; // long pointer used to process reshuffled data output
        ptrlongReshuffledData = reinterpret_cast<uint64*>(m_ptrshReshuffledData);

        //go through all bytes
        for(int32 i=0;i<m_nImgDataLength;i++)
        {                
            chTempByte = m_ptrchReorderedData[i];

            lLookupVal = m_vReshuffleLUT[(chTempByte >> 4) & 15]; // Check LUT using upper 4 bits

            ptrlongReshuffledData[nPos] <<= 1;
            ptrlongReshuffledData[nPos] |= lLookupVal;
            nPos++;

            lLookupVal = m_vReshuffleLUT[chTempByte & 15]; // Check LUT using lower 4 bits
	    
            ptrlongReshuffledData[nPos] <<= 1;
            ptrlongReshuffledData[nPos] |= lLookupVal;
            nPos++;
                
            //for each 32, the nPos needs to be reset to get back to the first pixel in the row
            if((i+1)%32==0)
            {
                //check if one complete row of pixels has been finished
                if((i+1)% nBytesInInputRow==0) nRow++;
                nPos = nRow*nLongsInOutputRow;
            }
        }         
    }
}

    
