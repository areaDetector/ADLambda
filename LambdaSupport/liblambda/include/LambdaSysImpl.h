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

#ifndef __LAMBDA_SYSTEM_IMPL_H__
#define __LAMBDA_SYSTEM_IMPL_H__

#include "LambdaGlobals.h"
#include "LambdaInterface.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    class LambdaConfigReader;
    class NetworkInterface;
    class ThreadPool;
    class FileOperation;
    class LambdaModule;
    class Task;
    template<class T>
        class MemPool;
 
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
	// When instantiating the detector control, the file path of the detector configuration directory should be provided.
        LambdaSysImpl(string _strConfigPath);
            
        /**
         * @brief destructor
         */
        ~LambdaSysImpl();
            
        //////////////////////////////////////////////////
        ///Inherited interface methods 
        //////////////////////////////////////////////////

        string GetSensorMaterial();

        vector<float> GetPosition();
        
        
	// Different operating modes (e.g. continuous read write, 24 bit) require different configuration files.
	// We store the config files for each mode in its own directory.
	// This command then sets the operating mode, using the files from the directory named strOperationMode.
        void SetOperationMode(string strOperationMode); 

        string GetOperationMode();

	// Controls how the detector is triggered. Currently 0-> detector takes images immediately, 1-> detector starts taking images when externally triggered
        void SetTriggerMode(short shTriggerMode);

        short GetTriggerMode();

	// Sets time per image in milliseconds
        void SetShutterTime(double dShutterTime);

        double GetShutterTime();

	// Sets time gap between images (when not in continuous mode) - also in ms.
        void SetDelayTime(double dDelayTime);

        double GetDelayTime();

	// Sets number of images to be taken in acquisition
        void SetNImages(long lImages);

        long GetNImages();
            
	// CURRENTLY UNUSED - TO BE DELETED. (This controlled a simple image writing function we used to test the library during development.)
        void SetSaveAllImages(bool bSaveAllImgs);

        bool GetSaveAllImages();

	// CURRENTLY UNUSED - TO BE DELETED. (For test purposes, this previously toggled between readout over 1G or 10G link.) 
        void SetBurstMode(bool bBurstMode);
        
        bool GetBurstMode();

	// Sets threshold number nThresholdNo to fEnergy (in keV). In standard operating modes, only threshold no. 0 is used.
        void SetThreshold(int nThresholdNo, float fEnergy);

        float GetThreshold(int nThresholdNo);

        void SetState(Enum_detector_state emState);
	// GetState() reads the state of the detector. The enum here follows Tango conventions - 0-3 correspond to ON (i.e. ready), DISABLE, RUNNING (i.e. busy), FAULT
        Enum_detector_state GetState();

	// CURRENTLY UNUSED - TO BE DELETED. (This controlled a simple image writing function we used to test the library during development.)
        void SetSaveFilePath(string strSaveFilePath);

        string GetSaveFilePath();

	// Path to the main directory where configuration files are contained.
        void SetConfigFilePath(string strConfigFilePath);

        string GetConfigFilePath();

	// Commands below start and stop imaging. "Start imaging" does not directly return a pointer to images.
	// When the detector is taking images, each decoded image is added to a queue.
	// GetQueueDepth returns the number of images available in the queue.
	// When the queue is not empty (>0), GetDecodedImageInt and GetDecodedImageShort functions below can be used to remove images from the queue
	// (in 12-bit mode and 24-bit mode respectively)

        void StartImaging();

        void StopImaging();

        vector<unsigned int> GetPixelMask();
        
        void SetDistortionCorrecttionMethod(int nMethod);
        int GetDistortionCorrecttionMethod();
        
	// Helper function to check the image size - nX and nY are x and y size in pixels, nImgDepth is the bit-depth (12 or 24).
        void GetImageFormat(int& nX, int& nY, int& nImgDepth);
        
	// Reads the ID of a chip within the module. Typically, chip 1 can be ID'd to uniquely identify a particular module.
	// Reading each chip on a module can also be used for debug purposes.
        string GetChipID(int chipNo);

	// CURRENTLY UNUSED
        long GetLatestImageNo();
	
    // Checks no of decoded images in queue.
        long GetQueueDepth();

	// CURRENTLY UNUSED - TO BE DELETED. (This allowed us to read raw, undecoded image data for test purposes.) 
        void GetRawImage(char* ptrchRetImg, long& lFrameNo,short& shErrCode);

	// In 24-bit mode, returns a pointer to the first image in the queue (int format) and removes the image from the queue
    // If first image is unavailable, returns null pointer
	// lFrameNo (pass by reference) returns the image number
	// shErrCode (pass by reference) returns 0 if image is OK, -1 if there was data loss
        int* GetDecodedImageInt(long& lFrameNo, short& shErrCode);

	// In 12-bit mode, returns a pointer to the first image in the queue (short format) and removes the image from the queue
	// If queue is empty, returns null pointer
	// lFrameNo (pass by reference) returns the image number
	// shErrCode (pass by reference) returns 0 if image is OK, -1 if there was data loss
        short* GetDecodedImageShort(long& lFrameNo, short& shErrCode);
            
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
        bool ReadConfig(bool bSwitchMode, string strOpMode);
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
         * @brief create task for threads(e.g. decode image)
         * @param strOpMode operation mode
         * @param strTaskName task name
         */
        void CreateTask(string strOpMode, string strTaskName);
        void CreateTask(string strOpMode, string strTaskName, Enum_priority enumTaskPriority);
        void CreateTask(string strOpMode,string strTaskName,NetworkInterface* objNetInterface);
	
        
      protected:
        ThreadPool* m_objThPool;
        MemPool<short> *m_objMemPoolDecodedShort;
        MemPool<int> *m_objMemPoolDecodedInt;
        MemPool<char> *m_objMemPoolRaw;
        vector<NetworkInterface*> m_vNetInterface;
        NetworkInterface* m_objUDPInterface;
        FileOperation* m_objFileOp;
        vector<short> m_vCurrentChip;
        vector<stMedipixChipData> m_vStCurrentChipData;
        LambdaModule* m_objLambdaModule;
        Task* m_objTask;
        LambdaConfigReader* m_objConfigReader;
        
        string m_strCurrentModuleName;	
        bool m_bAcquisitionStart;
        bool m_bAcquisitionStop;
            
        //////////////////////////////////////////////////
        ///System parameters
        //////////////////////////////////////////////////
        string m_strOperationMode;
        short m_shTriggerMode;
        double m_dShutterTime;
        double m_dDelayTime;
        long m_lFrameNo;
        bool m_bSaveAllImg;
        bool m_bBurstMode;
        int m_nThreshold;
        float m_fEnergy;
        Enum_detector_state m_emState;
        string m_strSaveFilePath;
        string m_strConfigFilePath;
        int m_nX;
        int m_nY;
        int m_nImgDepth;

        int m_nDistortedX;
        int m_nDistortedY;
        int m_nDistortedImageSize;
        int m_nDistortionCorrectionMethod;
        
        vector<unsigned int> m_vUnPixelMask;
        vector<int> m_vNIndex;
        vector<int> m_vNNominator;
        vector<float> m_vfTranslation;
        
        long m_lLastestImgNo;
        long m_lImgNo;
        long m_lQueueDepth;
        long m_lQueueImgNo;
        short m_shQueueImgErrCode;
        int* m_ptrnDecodeImg;
        short* m_ptrshDecodeImg;
        bool m_bRunning;
        bool m_bSysExit;
        vector<float> m_vThreshold;

        stDetCfgData m_stDetCfgData;
          
        int m_nModuleType;
        int m_nImgDataSize;
        int m_nSubImgNo;
        int m_nPacketsNumber;
        int m_nTaskID;

        int m_nThreadNumbers;
        int m_nRawBufferLength;
        int m_nDecodedBufferLength;
	int m_nDecodingThreads;
	int m_nLowPriorityDecodingThreads;

        bool m_bMultilink;

        ///structure
        vector< vector<string> > m_vStrMAC;
        vector< vector<string> > m_vStrIP;
        vector<unsigned short> m_vUShPort;
        string m_strTCPIPAddress;
        short m_shTCPPortNo;

        ///boost mutex
        boost::mutex m_bstMtxSync;
        
    };///end of class LambdaSysImpl    
}///end of namespace DetCommonNS


#endif
