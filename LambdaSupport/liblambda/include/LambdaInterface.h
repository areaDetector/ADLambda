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

///namespace DetCommonNS
namespace DetLambdaNS
{
    /**
     * @brief Lambda Interface class
     * This class provides the general interface for other applications(e.g. Tango)
     */
    class LambdaInterface
    {
      public:
        /**
         * @brief get sensor material
         * @return sensor matetrial
         */
        virtual string GetSensorMaterial() = 0;

        /**
         * @brief get module ID
         * @return module ID
         */
        virtual string GetModuleID() = 0;

        /**
         * @brief get system info. Includes software version, firmware version
         * @return system info
         */
        virtual string GetSystemInfo() = 0;

        /**
         * @brief get detector core library version
         * @return core lib version
         */
        virtual string GetDetCoreVersion() = 0;

        /**
         * @brief get liblambda version
         * @return liblamba version
         */
        virtual string GetLibLambdaVersion() = 0;

        /**
         * @brief get firmware version
         * @return firmware version
         */
        virtual string GetFirmwareVersion() = 0;

        /**
         * @brief get path of calib file
         */
        virtual string GetCalibFile() = 0;

        /**
         * @brief translation information for multi module system
         * @return translation information vector index: 0:x;1:y;2:z
         */
        virtual vector<float> GetPosition() = 0;

        /**
         * @brief set the operation mode
         * @param strOperationMode operation mode
         */
        virtual void SetOperationMode(string strOperationMode) = 0;

        /**
         * @brief get the operation mode
         * @return operation mode
         */
        virtual string GetOperationMode() = 0;

        /**
         * @brief set the trigger mode
         * @param sTriggerMode trigger mode
         */
        virtual void SetTriggerMode(int16 sTriggerMode) = 0;

        /**
         * @brief get the trigger mode
         * @return trigger mode
         */
        virtual int16 GetTriggerMode() = 0;

        /**
         * @brief set the shutter time
         * @param dShutterTime shutter time
         */
        virtual void SetShutterTime(double dShutterTime) = 0;

        /**
         * @brief get the shutter time
         * @return shutter time
         */
        virtual double GetShutterTime() = 0;

        /**
         * @brief set the delay time
         * @param dDelayTime delay time
         */
        virtual void SetDelayTime(double dDelayTime) = 0;

        /**
         * @brief get the delay time
         * @return delay time
         */
        virtual double GetDelayTime() = 0;

        /**
         * @brief set number of images that the detector should take during acquisition
         * @param lImages number of images
         */
        virtual void SetNImages(int32 lImages) = 0;

        /**
         * @brief get the number of images that the detector will take
         * @return number of images
         */
        virtual int32 GetNImages() = 0;

        /**
         * @brief set saving all images or not
         * @param bSaveAllImgs true:save all images = 0; false: do not save
         */
        virtual void SetSaveAllImages(bool bSaveAllImgs) = 0;

        /**
         * @brief get the value of saving all images or not
         * @return value of Saving all images
         */
        virtual bool GetSaveAllImages() = 0;

        /**
         * @brief set burst mode.  using 10GE links or not.
         * @param bBurstMode true: 10GE link. false: 1GE link
         */
        virtual void SetBurstMode(bool bBurstMode) = 0;

        /**
         * @brief get the value of burst mode
         * @return value of burst mode
         */
        virtual bool GetBurstMode() = 0;

        /**
         * @brief set threshold
         * Lambda is a photon-counting detector(like Pilatus).
         * Depending on the opration mode @see GetOperationMode(),
         * multiple thresholds(numbered 0-7) are possible.
         * The nThresholdNo chooses which to change
         * @param nThresholdNo threshold number
         * @param fEnergy energy value
         */
        virtual void SetThreshold(int32 nThresholdNo, float fEnergy) = 0;

        /**
         * @brief get threshold
         * @param nThresholdNo chooses which to get
         * @return value of energy
         */
        virtual float GetThreshold(int32 nThresholdNo) = 0;

        /**
         * @brief set state
         * @param emState status
         */
        virtual void SetState(Enum_detector_state emState) = 0;

        /**
         * @brief Get States of the detector
         * @return value of state(ON,DISABLE,RUNNING,FAULT)
         */
        virtual Enum_detector_state GetState() = 0;

        /**
         * @brief set the file path for saving images
         * @param strSaveFilePath file path
         */
        virtual void SetSaveFilePath(string strSaveFilePath) = 0;

        /**
         * @brief get the file path for saving images
         * @return file path
         */
        virtual string GetSaveFilePath() = 0;

        /**
         * @brief set the config file path
         * @param strConfigFilePath config file path
         */
        virtual void SetConfigFilePath(string strConfigFilePath) = 0;

        /**
         * @brief get the config file path
         * @return config file path
         */
        virtual string GetConfigFilePath() = 0;

        /**
         * @brief start acquisition
         */
        virtual void StartImaging() = 0;

        /**
         * @brief stop acquisition
         */
        virtual void StopImaging() = 0;

        /**
         * @brief set compression method
         * @param the method used
         *        0 : disabled
         *        1 : software deflate
         *        2: hardware deflate
         * @param compression level default value is 2
         */
        virtual void SetCompressionMethod(int32 nMethod,int32 nCompLevel = 2) = 0;

        /**
         * @brief get compression method
         * @return the method used
         *         0 : disabled
         *         1 : software deflate
         *         2: hardware deflate
         */
        virtual int32 GetCompressionMethod() = 0;

        /**
         * @brief get pixel mask
         * @return pixel mask array
         */
        virtual vector<uint32> GetPixelMask() = 0;

        /**
         * @brief distortion correction method
         * @param nMethod  method applied \n
         *        0 : no distortion correction
         *        1 : division
         */
        virtual void SetDistortionCorrecttionMethod(int32 nMethod) = 0;

        /**
         * @brief get distortion correction method
         * @return the method used
         *         0 : disabled
         *         1 : division
         */
        virtual int32 GetDistortionCorrecttionMethod() = 0;

        /**
         * @brief get image format
         * @param nX size x
         * @param nY size y
         * @param nImgDepth depth of image(12 or 24)
         */
        virtual void GetImageFormat(int32& nX, int32& nY, int32& nImgDepth) = 0;

        /**
         * @brief get latest image No.
         * @return latest image No.
         */
        virtual int32 GetLatestImageNo() = 0;

        /**
         * @brief get the image numbers in the queue
         * @return image numbers
         */
        virtual int32 GetQueueDepth() = 0;

        /**
         * @brief get size of free buffer for raw images
         * @return free buffer for raw images
         */
        virtual int32 GetFreeBufferSize() = 0;
        /**
         * @brief get the raw image
         * @return raw image data
         */
        virtual void GetRawImage(char* ptrchRetImg, int32& lFrameNo,int16& shErrCode) = 0;

        /**
         * @brief get decoded image
         * @param lFrameNo frame no
         * @param shErrCode error code
         * @return int*(24bit image)
         */
        virtual int32* GetDecodedImageInt(int32& lFrameNo, int16& shErrCode) = 0;

        /**
         * @brief get decoded image
         * @param lFrameNo frame no
         * @param shErrCode error code
         * @return short*(12bit image)
         */

        virtual int16* GetDecodedImageShort(int32& lFrameNo, int16& shErrCode) = 0;
        
        /**
         * @brief get compressed data
         * @param frame no
         * @param error code
         * @param data length
         * @return char*(8bit data)
         */
        virtual char* GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength) = 0;

        /**
         * @brief get current image for live view
         * @param lFrameNo frame no
         * @param shErrCode error code
         * @return image data
         */
        virtual int32* GetCurrentImage(int32& lFrameNo,int16& shErrCode) = 0;
        
        /**
         * @brief get no of subimages for each image taken (e.g. in multiple threshold modes)
         * @return int
         */
        virtual int32 GetNSubimages() = 0;

        /**
         * @brief destructor
         */
        virtual ~LambdaInterface(){};

      protected:
        /**
         * @brief contrunctor
         */
        LambdaInterface(){};
    };///end of class LambdaInterface
}
