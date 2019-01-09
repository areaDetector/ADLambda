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
#include "LambdaInterface.h"

#include <fsdetector/core/NetworkInterface.h>
#include <fsdetector/core/NetworkImplementation.h>
#include <fsdetector/core/Utils.h>
#include <fsdetector/core/FilesOperation.h>
#include <fsdetector/core/ThreadUtils.h>
#include <fsdetector/core/MemUtils.h>

namespace DetLambdaNS
{
    class LambdaConfigReader;
    class LambdaModule;
    class ImageDecoder;  
 
    /**
     * @brief Lambda system implementation class
     * This class implements all the interface in @see LambdaInterface
     */
    class LambdaSysImpl : public LambdaInterface
    {
      public:
        /**
         * @brief constructor
         */
        LambdaSysImpl();

        /**
         * @brief constructor
         * @param _strConfigPath config file path
         */
        // When instantiating the detector control,
        // the file path of the detector configuration directory should be provided.
        LambdaSysImpl(string _strConfigPath);
            
        /**
         * @brief destructor
         */
        ~LambdaSysImpl();
            
        //////////////////////////////////////////////////
        ///Inherited interface methods 
        //////////////////////////////////////////////////

        string GetSensorMaterial();

        string GetModuleID();
        string GetSystemInfo();

        string GetDetCoreVersion();
        string GetLibLambdaVersion();
        string GetFirmwareVersion();
        
        
        string GetCalibFile();
        
        vector<float> GetPosition();
        
        
        // Different operating modes (e.g. continuous read write, 24 bit)
        // require different configuration files.
        // We store the config files for each mode in its own directory.
        // This command then sets the operating mode,
        // using the files from the directory named strOperationMode.
        void SetOperationMode(string strOperationMode); 

        string GetOperationMode();

        // Controls how the detector is triggered.
        // Currently 0-> detector takes images immediately,
        // 1-> detector starts taking images when externally triggered
        void SetTriggerMode(int16 shTriggerMode);

        int16 GetTriggerMode();

        // Sets time per image in milliseconds
        void SetShutterTime(double dShutterTime);

        double GetShutterTime();

        // Sets time gap between images (when not in continuous mode) - also in ms.
        void SetDelayTime(double dDelayTime);

        double GetDelayTime();

        // Sets number of images to be taken in acquisition
        void SetNImages(int32 lImages);

        int32 GetNImages();

        // CURRENTLY UNUSED - TO BE DELETED.
        // (This controlled a simple image writing function
        // we used to test the library during development.)
        void SetSaveAllImages(bool bSaveAllImgs);

        bool GetSaveAllImages();
            
        void SetBurstMode(bool bBurstMode);
        
        bool GetBurstMode();

        // Sets threshold number nThresholdNo to fEnergy (in keV).
        // In standard operating modes, only threshold no. 0 is used.
        void SetThreshold(int32 nThresholdNo, float fEnergy);

        float GetThreshold(int32 nThresholdNo);

        void SetState(Enum_detector_state emState);
        // GetState() reads the state of the detector.
        // The enum here follows Tango conventions -
        // 0-3 correspond to ON (i.e. ready), DISABLE, RUNNING (i.e. busy), FAULT
        Enum_detector_state GetState();

        // CURRENTLY UNUSED - TO BE DELETED.
        // (This controlled a simple image writing function
        // we used to test the library during development.)
        void SetSaveFilePath(string strSaveFilePath);

        string GetSaveFilePath();

        // Path to the main directory where configuration files are contained.
        void SetConfigFilePath(string strConfigFilePath);

        string GetConfigFilePath();

        // Commands below start and stop imaging.
        // "Start imaging" does not directly return a pointer to images.
        // When the detector is taking images, each decoded image is added to a queue.
        // GetQueueDepth returns the number of images available in the queue.
        // When the queue is not empty (>0),
        // GetDecodedImageInt and GetDecodedImageShort functions below
        // can be used to remove images from the queue
        // (in 12-bit mode and 24-bit mode respectively)

        void StartImaging();

        void StopImaging();

        void SetCompressionEnabled(bool bCompressionEnabled,int32 nCompLevel); 
        void GetCompressionEnabled(bool& bCompressionEnabled,int32& nCompLevel);
        int32 GetCompressionMethod();
        
        vector<uint32> GetPixelMask();
        
        void SetDistortionCorrecttionMethod(int32 nMethod);
        int32 GetDistortionCorrecttionMethod();
        
        // Helper function to check the image size -
        // nX and nY are x and y size in pixels, nImgDepth is the bit-depth (12 or 24).
        void GetImageFormat(int32& nX, int32& nY, int32& nImgDepth);

        // Helper function to check the number of subimages (e.g. when using multiple thresholds
        int32 GetNSubimages();

        // Helper function to get enum specifying Medipix3 readout chip mode
        Enum_readout_mode GetReadoutModeCode();
	
        // // Reads the ID of a chip within the module.
        // // Typically, chip 1 can be ID'd to uniquely identify a particular module.
        // // Reading each chip on a module can also be used for debug purposes.
        // string GetChipID(int32 chipNo);

        // CURRENTLY UNUSED
        int32 GetLatestImageNo();
	
        // Checks no of decoded images in queue.
        int32 GetQueueDepth();

        int32 GetFreeBufferSize();

        // CURRENTLY UNUSED - TO BE DELETED.
        // (This allowed us to read raw, undecoded image data for test purposes.) 
        void GetRawImage(char* ptrchRetImg, int32& lFrameNo,int16& shErrCode);

        // In 24-bit mode, returns a pointer to the first image in the queue (int format) and
        // removes the image from the queue
        // If first image is unavailable, returns null pointer
        // lFrameNo (pass by reference) returns the image number
        // shErrCode (pass by reference) returns 0 if image is OK, -1 if there was data loss
        int32* GetDecodedImageInt(int32& lFrameNo, int16& shErrCode);

        // In 12-bit mode, returns a pointer to the first image
        // in the queue (short format) and removes the image from the queue
        // If queue is empty, returns null pointer
        // lFrameNo (pass by reference) returns the image number
        // shErrCode (pass by reference) returns 0 if image is OK, -1 if there was data loss
        int16* GetDecodedImageShort(int32& lFrameNo, int16& shErrCode);
        
        char* GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength);

        void SetCurrentImage(int16* ptrshImg,int32 lFrameNo,int16 shErrCode);
        void SetCurrentImage(int32* ptrnImg,int32 lFrameNo,int16 shErrCode);
        int32* GetCurrentImage(int32& lFrameNo,int16& shErrCode);
            
        // These check on the status of certain internal variables for test purposes.
        bool GetAcquisitionStart() const;
        bool GetAcquisitionStop() const;

        void SetAcquisitionStart(const bool bStatus);
        void SetAcquisitionStop(const bool bStatus);
        bool SysExit() const;
        
        
      protected:
        /**
         * @brief init all lambda sys
         */
        void InitSys();

        /**
         * @brief several help methods for initializing system
         */
        void ReadConfig(bool bSwitchMode, string strOpMode);
        bool InitNetwork();
        void InitThreadPool();
        void InitMemoryPool();
            
        /**
         * @brief exit system
         */
        void ExitSys();
        
        /**
         * @brief check file exists
         * @param strFileName file name
         * @return true file exists;flase file does not exists
         */
        bool FileExists(string strFileName);

        /**
         * @brief get Medipix3 readout mode from operating mode string name -
         * initial hard-coded approach for creating new op modes
         * @param strOpMode op mode name
         * @return true file exists;flase file does not exists
         */
        Enum_readout_mode GetReadoutModeEnum(string strOperationMode);
	
        /**
         * @brief create task for threads(e.g. decode image)
         * @param strOpMode operation mode
         * @param strTaskName task name
         */
        void CreateTask(string strOpMode, string strTaskName);
        void CreateTask(string strOpMode, string strTaskName, Enum_priority enumTaskPriority);
        void CreateTask(string strOpMode,string strTaskName,NetworkInterface* objNetInterface);

        /**
         * @brief join all decode tasks
         */
        void JoinAllDecodeTasks();
	
        /**
         * @brief stop all decode tasks
         */
        void StopAllDecodeTasks();
	
      protected:
        ThreadPool* m_objThPool;
        MemPool<int16> *m_objMemPoolDecodedShort;
        MemPool<int32> *m_objMemPoolDecodedInt;
        MemPool<char> *m_objMemPoolRaw;
        MemPool<char> *m_objMemPoolCompressed;
        vector<NetworkInterface*> m_vNetInterface;
        NetworkInterface* m_objUDPInterface;
        FileOperation* m_objFileOp;
        vector<int16> m_vCurrentChip;
        vector<stMedipixChipData> m_vStCurrentChipData;
        LambdaModule* m_objLambdaModule;
        Task* m_objTask;
        LambdaConfigReader* m_objConfigReader;
        
        string m_strCurrentModuleName;	
        string m_strSystemType;
        bool m_bAcquisitionStart;
        bool m_bAcquisitionStop;
            
        //////////////////////////////////////////////////
        ///System parameters
        //////////////////////////////////////////////////
        string m_strOperationMode;
        int16 m_shTriggerMode;
        double m_dShutterTime;
        double m_dDelayTime;
        int32 m_lFrameNo;
        bool m_bSaveAllImg;
        bool m_bBurstMode;
        int32 m_nThreshold;
        float m_fEnergy;
        Enum_detector_state m_emState;
        Enum_readout_mode m_emReadoutMode;
        string m_strSaveFilePath;
        string m_strConfigFilePath;
        int32 m_nX;
        int32 m_nY;
        int32 m_nImgDepth;

        int32 m_nDistortedX;
        int32 m_nDistortedY;
        int32 m_nDistortedImageSize;
        int32 m_nDistortionCorrectionMethod;

        bool m_bSlaveModule;
	
        vector<uint32> m_vUnPixelMask;
        vector<int32> m_vNIndex;
        vector<int32> m_vNNominator;
        vector<float> m_vfTranslation;
        
        int32 m_lLastestImgNo;
        int32 m_lImgNo;
        int32 m_lQueueDepth;
        int32 m_lQueueImgNo;
        int16 m_shQueueImgErrCode;
        int32* m_ptrnDecodeImg;
        int16* m_ptrshDecodeImg;

        int32* m_ptrnLiveData;
        int32 m_lLiveFrameNo;
        int16 m_shLiveErrCode;
	bool m_hasFirstImageArrived;
        
        bool m_bRunning;
        bool m_bSysExit;
        vector<float> m_vThreshold;

        stDetCfgData m_stDetCfgData;
          
        int32 m_nModuleType;
        int32 m_nImgDataSize;
        int32 m_nSubimages;
        int32 m_nPacketsNumber;
        int32 m_nTaskID;

        int32 m_nThreadNumbers;
        int32 m_nRawBufferLength;
        int32 m_nDecodedBufferLength;
        int32 m_nDecodingThreads;
        vector<int32> m_vNDecodingThreads;
        double m_dCriticalShutterTime;

        bool m_bMultilink;

        ///structure
        vector< vector<string> > m_vStrMAC;
        vector< vector<string> > m_vStrIP;
        vector<uint16> m_vUShPort;
        string m_strTCPIPAddress;
        int16 m_shTCPPortNo;

        //compression
        bool m_bCompressionEnabled;
        int32 m_nCompressionLevel;
        int32 m_nCompressionMethod;

        char* m_ptrchData;

        string m_strModuleID;
        
        ///boost mutex
        boost::mutex m_bstMtxSync;
    };///end of class LambdaSysImpl    
}
