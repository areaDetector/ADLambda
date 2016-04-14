/*
 * (c) Copyright 2014-2015 DESY, David Pennicard <david.pennicard@desy.de>
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
 *     Author: David Pennicard <david.pennicard@desy.de>
 */

#ifndef __DISTORTION_CORRECTOR_H__
#define __DISTORTION_CORRECTOR_H__

#include "Globals.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    /**
     * @brief class DistortionCorrector
     */
    template <typename T> 
        class DistortionCorrector
    {

      public:
        /**
         * @brief constructor of given type
         * @param _vIndex: vector saying where each output pixel's data comes from.
         * @param _vNominator: specifies what should be done with each output pixel (e.g. blank)
         * @param _nSaturatedPixel : saturated pixel value
         */
        DistortionCorrector(vector<int> _vIndex, vector<int> _vNominator,int _nSaturatedPixel)
            :m_nSaturatedPixel(_nSaturatedPixel)
        {
            // Set up lookup tables 
            m_vIndex = _vIndex;
            m_vNominator = _vNominator;
            m_outputLength = m_vIndex.size();
            // Possibly set up error checking for indices to avoid segfaults?
            // Setup variable for output
            m_ptrCorrectedImage = new T[m_outputLength];
        }

        /**
         * @brief destructor
         */
        ~DistortionCorrector()
        {
            delete [] m_ptrCorrectedImage;
        }

        /**
         * @brief Run distortion correction
         * @param Pointer to input image
         * @return Pointer to output image (within corrector)
         */
        T* RunDistortCorrect(T* ptrInput)
        {
            // Possibly add in error checking here, e.g. segfault avoidance
      
            int nomVal;
            int indexVal;
  
            for(int j=0;j<m_outputLength;j++)
            {
                nomVal = m_vNominator[j];
		if (nomVal == 1) m_ptrCorrectedImage[j] = ptrInput[m_vIndex[j]]; // Grab data from input pixel given by index                
                else
                {
		    if (nomVal <= 0) m_ptrCorrectedImage[j] = nomVal;
                    else
                    {
                        //for saturated pixel, do not apply division
                        if(ptrInput[m_vIndex.at(j)] != m_nSaturatedPixel)
                            m_ptrCorrectedImage[j] = round((ptrInput[m_vIndex[j]]*1.0) / nomVal);  // Positive nominator implies extra-large pixel, so divide
                        else
                            m_ptrCorrectedImage[j] = ptrInput[m_vIndex[j]];
                    }
                    
                }
            }
            return m_ptrCorrectedImage;
        }
            
        
      private:
        T* m_ptrCorrectedImage;
        vector<int> m_vIndex;
        vector<int> m_vNominator;
        int m_outputLength;
        int m_nSaturatedPixel;

    };///end of class DistortionCorrector

}///end of namespace DetCommonNS


#endif 
