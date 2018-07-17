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

#include "LambdaGlobals.h"
#include "LambdaSysImpl.h"
#include "LambdaModule.h"
#include "ImageDecoder.h"
#include "LambdaTask.h"
#include "LambdaConfigReader.h"
#include "LambdaDataRecvFactory.h"
#include "LambdaDataReceiver.h"

namespace DetLambdaNS
{
    LambdaSysImpl::LambdaSysImpl()
    {
        LOG_TRACE(__FUNCTION__);;
    }

    LambdaSysImpl::LambdaSysImpl(string _strConfigPath,Enum_log_level _level)
        :m_strConfigFilePath(_strConfigPath)
    {
        InitLogLevel(_level);
        LOG_TRACE(__FUNCTION__);

        m_emState = BUSY;
        SetState(BUSY);
        InitSys();
        SetState(ON);
    }

    LambdaSysImpl::~LambdaSysImpl()
    {
        LOG_TRACE(__FUNCTION__);
        ExitSys();
    }

    void LambdaSysImpl::InitSys()
    {
        LOG_TRACE(__FUNCTION__);

        float fDefaultEnergy = 6.0;
        m_strCurrentModuleName = "";
        m_strOperationMode= "";
        m_shTriggerMode = 0;
        m_dShutterTime = 2000;
        m_dDelayTime = 0;
        m_lFrameNo = 1;
        m_bSaveAllImg = false;
        m_bBurstMode = true;
        m_nThreshold = 0;
        m_fEnergy = fDefaultEnergy;
        //m_nState = 0;
        m_strSaveFilePath = "./";
        //m_strConfigFilePath = "";
        m_nX = -1;
        m_nY = -1;
        m_nSubimages = -1;
        m_nImgDepth = -1;

        m_bSlaveModule = false;

        m_lLastestImgNo = -1;
        m_lImgNo = -1;
        m_lQueueDepth = -1;
        m_lQueueImgNo = -1;
        m_vThreshold.resize(8,fDefaultEnergy);


        m_objTask = NULL;
        m_nModuleType = 0;
        m_nTaskID = 0;
        m_bRunning = false;
        m_bSysExit = false;

        m_bCompressionEnabled = true;
        m_nCompressionLevel = 2;
        m_nCompressionMethod = 0;

        m_nDistortionCorrectionMethod = 1;

        m_strModuleID = "unknown";
        m_strSystemType = "standard";

        m_objConfigReader = new LambdaConfigReader(m_strConfigFilePath);
        m_objConfigReader->LoadLocalConfig(false, "");

        InitDetectorSettings();

        //get fisrt chip id
        m_strModuleID = m_objModuleCtrl->GetChipID(1);

        m_bRunning = true;
    }

    void LambdaSysImpl::ExitSys()
    {
        LOG_TRACE(__FUNCTION__);

        delete m_objTCP;
        m_objTCP = NULL;

        m_objDataRecv.reset(nullptr);
        m_objModuleCtrl.reset(nullptr);

        delete m_objConfigReader;
        m_objConfigReader = NULL;

        m_objFileOp = NULL;
    }

    void LambdaSysImpl::ReadConfig()
    {
        LOG_TRACE(__FUNCTION__);

        m_objConfigReader->GetTCPConfig(m_strTCPIPAddress,m_shTCPPortNo);
        m_strOperationMode = m_objConfigReader->GetOperationMode();
        m_strCurrentModuleName = m_objConfigReader->GetModuleName();
        m_vfTranslation = m_objConfigReader->GetPosition();
        m_emReadoutMode = GetReadoutModeEnum(m_strOperationMode);
        m_vUnPixelMask = m_objConfigReader->GetPixelMask();
    }

    string LambdaSysImpl::GetSensorMaterial()
    {
        LOG_TRACE(__FUNCTION__);

        if(m_strCurrentModuleName.find("FullRX")!=std::string::npos)
            return string("Si");
        else if(m_strCurrentModuleName.find("GaAs")!=std::string::npos)
            return string("GaAs");
        else if(m_strCurrentModuleName.find("CdTe")!=std::string::npos)
            return string("CdTe");
        else
            return string("UNKNOWN");
    }

    string LambdaSysImpl::GetModuleID()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strModuleID;
    }

    string LambdaSysImpl::GetSystemInfo()
    {
        LOG_TRACE(__FUNCTION__);

        return string("liblambda version:")
            + LAMBDA_VERSION
            + string(" -- firmware version:")
            + m_objModuleCtrl->GetFirmwareVersion();
    }

    string LambdaSysImpl::GetDetCoreVersion()
    {
        LOG_TRACE(__FUNCTION__);

        return FSDetCoreNS::FSDETCORE_VERSION;
    }

    string LambdaSysImpl::GetLibLambdaVersion()
    {
        LOG_TRACE(__FUNCTION__);

        return LAMBDA_VERSION;
    }

    string LambdaSysImpl::GetFirmwareVersion()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objModuleCtrl->GetFirmwareVersion();
    }

    string LambdaSysImpl::GetCalibFile()
    {
        LOG_TRACE(__FUNCTION__);

        return m_strConfigFilePath+string("/")+m_strCurrentModuleName+string("/lookups/")
            +m_strCurrentModuleName+string("_calib.nxs");
    }
    vector<float> LambdaSysImpl::GetPosition()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vfTranslation;
    }

    Enum_readout_mode LambdaSysImpl::GetReadoutModeEnum(string strOperationMode)
    {
        Enum_readout_mode emNewMode;
        LOG_TRACE(__FUNCTION__);
        // For time being, hard-code options -
        // in future, should make this more flexible (or carefully tailored)
        if(strOperationMode=="ContinuousReadWrite") emNewMode = OPERATION_MODE_12;
        else if(strOperationMode=="TwentyFourBit") emNewMode = OPERATION_MODE_24;
        else if(strOperationMode=="DualThreshold") emNewMode = OPERATION_MODE_2x12;
        else if(strOperationMode=="ContinuousReadWriteCSM") emNewMode = OPERATION_MODE_12;
        else if(strOperationMode=="TwentyFourBitCSM") emNewMode = OPERATION_MODE_24;
        else if(strOperationMode=="DualThresholdCSM") emNewMode = OPERATION_MODE_2x12;
        else if(strOperationMode=="HighSpeedBurst") emNewMode = OPERATION_MODE_12;
        else emNewMode = OPERATION_MODE_UNKNOWN;
        return emNewMode;
    }

    void LambdaSysImpl::SetOperationMode(string strOperationMode)
    {
        LOG_TRACE(__FUNCTION__);
        // In initial implementation, hard-code these

        m_emReadoutMode = GetReadoutModeEnum(strOperationMode);

        SetState(BUSY);
        if(m_emReadoutMode == OPERATION_MODE_UNKNOWN)
            LOG_STREAM(__FUNCTION__,ERROR,
                    ("This Operation Mode( "+strOperationMode +") does not exist!"));
        else if(strOperationMode == m_strOperationMode)
            LOG_INFOS(("The current operation mode is already" + m_strOperationMode));
        else
        {
            m_objConfigReader->LoadLocalConfig(true, strOperationMode);
            InitDetectorSettings();
        }

        SetState(ON);
    }

    string LambdaSysImpl::GetOperationMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strOperationMode;
    }

    void LambdaSysImpl::SetTriggerMode(int16 shTriggerMode)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_shTriggerMode = shTriggerMode;
        m_objModuleCtrl->WriteTriggerMode(shTriggerMode);
        SetState(ON);
    }

    int16 LambdaSysImpl::GetTriggerMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_shTriggerMode;
    }

    void LambdaSysImpl::SetShutterTime(double dShutterTime)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_dShutterTime = dShutterTime;
        m_objModuleCtrl->WriteShutterTime(dShutterTime);
        m_objDataRecv->SetShutterTime(dShutterTime);
        SetState(ON);
    }

    double LambdaSysImpl::GetShutterTime()
    {
        LOG_TRACE(__FUNCTION__);
        return m_dShutterTime;
    }

    void LambdaSysImpl::SetDelayTime(double dDelayTime)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_dDelayTime = dDelayTime;
        m_objModuleCtrl->WriteDelayTime(dDelayTime);
        SetState(ON);
    }

    double LambdaSysImpl::GetDelayTime()
    {
        LOG_TRACE(__FUNCTION__);
        return m_dDelayTime;
    }

    void LambdaSysImpl::SetNImages(int32 lImages)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_lFrameNo = lImages;
        m_objModuleCtrl->WriteImageNumbers(lImages);
        SetState(ON);
    }

    int32 LambdaSysImpl::GetNImages()
    {
        LOG_TRACE(__FUNCTION__);
        return m_lFrameNo;
    }

    void LambdaSysImpl::SetSaveAllImages(bool bSaveAllImgs)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_bSaveAllImg = bSaveAllImgs;
        SetState(ON);
    }

    bool LambdaSysImpl::GetSaveAllImages()
    {
        LOG_TRACE(__FUNCTION__);
        return m_bSaveAllImg;
    }

    void LambdaSysImpl::SetBurstMode(bool bBurstMode)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_bBurstMode = bBurstMode;
        int nVal = (bBurstMode==true?1:0);
        m_objModuleCtrl->WriteNetworkMode(nVal);
        SetState(ON);
    }

    bool LambdaSysImpl::GetBurstMode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_bBurstMode;
    }

    void LambdaSysImpl::SetThreshold(int32 nThresholdNo, float fEnergy)
    {
        LOG_TRACE(__FUNCTION__);

        SetState(BUSY);
        m_nThreshold = nThresholdNo;
        m_vThreshold[nThresholdNo] = fEnergy;
        m_fEnergy = fEnergy;
        m_objModuleCtrl->WriteEnergyThreshold(nThresholdNo,fEnergy);
        SetState(ON);
    }

    float LambdaSysImpl::GetThreshold(int32 nThresholdNo)
    {
        LOG_TRACE(__FUNCTION__);
        m_nThreshold = nThresholdNo;
        m_fEnergy = m_vThreshold[nThresholdNo];
        return m_vThreshold[nThresholdNo];
    }

    void LambdaSysImpl::SetState(Enum_detector_state emState)
    {
        LOG_TRACE(__FUNCTION__);

        // When exiting RECEIVING_IMAGES state,
        // we want to make sure detector module is ready for next acquisition
        if(m_emState == RECEIVING_IMAGES && emState != RECEIVING_IMAGES)
            m_objModuleCtrl->PrepNextImaging();

        m_emState = emState;
    }

    Enum_detector_state LambdaSysImpl::GetState()
    {
        // If imaging is running, need to check on progress here and update if necessary
        // States 4 and above are RECEIVING_IMAGES, PROCESSING_IMAGES, FINISHED
        if (m_emState >= 4)
        {
            //Check for raw images complete
            int32 expectedframes = m_objDataRecv->GetExpectedDecodedImages();
            int32 expectedRawFrames = m_objDataRecv->GetExpectedRawImages();

            int32 rawframes = m_objDataRecv->GetReceivedRawImages();

            if(rawframes<expectedRawFrames)
                m_emState = RECEIVING_IMAGES;
            else
            {
                int32 decodedframes = 0;

                decodedframes = m_objDataRecv->GetReceivedDecodedImages();

                if((decodedframes < expectedframes)||(m_objDataRecv->GetQueueDepth()>0))
                {
                    if (m_emState != 5)
                    {
                        m_emState = PROCESSING_IMAGES;
                        // Allow low priority tasks to run if image collection is
                        // done but we have images to process
                        m_objDataRecv->SetPriorityLevel(LOW);
                    }
                }
                else
                    // This indicates library done, BUT that we haven't reset state
                    // to ON with StopFastImaging.
                    // The FINISHED state is to help Tango -
                    // it allows the library to remain in a "not ON" state until
                    // the Tango server is finished with files etc.
                    m_emState = FINISHED;
            }
        }

        // cout << "Current state is " << m_emState << "\n";

        return m_emState;
    }


    void LambdaSysImpl::SetSaveFilePath(string strSaveFilePath)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_strSaveFilePath = strSaveFilePath;
        SetState(ON);
    }

    string LambdaSysImpl::GetSaveFilePath()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strSaveFilePath;
    }

    void LambdaSysImpl::SetConfigFilePath(string strConfigFilePath)
    {
        LOG_TRACE(__FUNCTION__);
        SetState(BUSY);
        m_strConfigFilePath = strConfigFilePath;
        SetState(ON);
    }

    string LambdaSysImpl::GetConfigFilePath()
    {
        LOG_TRACE(__FUNCTION__);
        return m_strConfigFilePath;
    }

    void LambdaSysImpl::StartImaging()
    {
        LOG_TRACE(__FUNCTION__);

        SetState(RECEIVING_IMAGES);

        m_objDataRecv->SetRequestedImages(m_lFrameNo);
        m_objDataRecv->Start();
        m_objModuleCtrl->StartFastImaging();
    }

    void LambdaSysImpl::StopImaging()
    {
        LOG_TRACE(__FUNCTION__);

        if(GetState() == RECEIVING_IMAGES)
            m_objModuleCtrl->StopFastImaging();

        m_objDataRecv->Stop();

        usleep(100);
        SetState(ON);
    }

    void LambdaSysImpl::SetCompressionMethod(int32 nMethod,int32 nCompLevel)
    {
        LOG_TRACE(__FUNCTION__);

        m_nCompressionMethod = nMethod;
        m_nCompressionLevel = nCompLevel;
        if(nMethod != 0)
            m_objDataRecv->EnableCompression(nMethod,nCompLevel);
        else
            m_objDataRecv->DisableCompression();
    }

    int32 LambdaSysImpl::GetCompressionMethod()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nCompressionMethod;
    }

    vector<uint32> LambdaSysImpl::GetPixelMask()
    {
        LOG_TRACE(__FUNCTION__);

        return m_vUnPixelMask;
    }

    void LambdaSysImpl::SetDistortionCorrecttionMethod(int32 nMethod)
    {
        LOG_TRACE(__FUNCTION__);

        m_nDistortionCorrectionMethod = nMethod;

        if(nMethod != 0)
            m_objDataRecv->EnableDistortionCorrection(nMethod);
        else
            m_objDataRecv->DisableDistiotionCorrection();
    }

    int32 LambdaSysImpl::GetDistortionCorrecttionMethod()
    {
        LOG_TRACE(__FUNCTION__);

        return m_nDistortionCorrectionMethod;
    }

    void LambdaSysImpl::GetImageFormat(int32& nX, int32& nY, int32& nImgDepth)
    {
        LOG_TRACE(__FUNCTION__);

        m_objDataRecv->GetImageInfo(nX,nY,nImgDepth);
    }

    int32 LambdaSysImpl::GetNSubimages()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetSubimages();
    }

    // Helper function - returns enum corresponding to Medipix3 chip readout mode
    Enum_readout_mode LambdaSysImpl::GetReadoutModeCode()
    {
        LOG_TRACE(__FUNCTION__);
        return m_emReadoutMode;
    }

    int32 LambdaSysImpl::GetLatestImageNo()
    {
        LOG_TRACE(__FUNCTION__);

        return m_lLastestImgNo;
    }

    int32 LambdaSysImpl::GetQueueDepth()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetQueueDepth();
    }

    int32 LambdaSysImpl::GetFreeBufferSize()
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetFreeBufferSize();
    }

    void LambdaSysImpl::GetRawImage(char* ptrchRetImg,int32& lFrameNo,int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);
    }

    int32* LambdaSysImpl::GetDecodedImageInt(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetDecodedImageInt(lFrameNo,shErrCode);
    }

    int16* LambdaSysImpl::GetDecodedImageShort(int32& lFrameNo, int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetDecodedImageShort(lFrameNo,shErrCode);
    }

    char* LambdaSysImpl::GetCompressedData(int32& lFrameNo,int16& shErrCode,int32& nDataLength)
    {
        LOG_TRACE(__FUNCTION__);

        return m_objDataRecv->GetCompressedData(lFrameNo,shErrCode,nDataLength);
    }

    int32* LambdaSysImpl::GetCurrentImage(int32& lFrameNo,int16& shErrCode)
    {
        LOG_TRACE(__FUNCTION__);

        // GET current image from data recv
        return m_objDataRecv->GetCurrentImage(lFrameNo,shErrCode);
    }

    bool LambdaSysImpl::InitControl()
    {
        LOG_TRACE(__FUNCTION__);
        if(m_objModuleCtrl)
            m_objModuleCtrl.reset(nullptr);

        m_objTCP = new NetworkTCPInterface(
            new NetworkTCPImplementation(m_strTCPIPAddress,m_shTCPPortNo));

        if(m_objTCP->Connect() != 0)
            return false;

        m_objModuleCtrl = uptr_module(new LambdaModule());

        if(m_objModuleCtrl->InitModule(m_objTCP,*m_objConfigReader))
        {
            m_objModuleCtrl->ConfigDataLink();
            return true;
        }
        else
            return false;

    }

    bool LambdaSysImpl::InitRecv()
    {
        LOG_TRACE(__FUNCTION__);
        LambdaDataRecvFactory datarecv_factory;
        if(m_objDataRecv)
            m_objDataRecv.reset(nullptr);

        m_objDataRecv = datarecv_factory.CreateDataReceiver(m_emReadoutMode);
        return m_objDataRecv->Init(m_dShutterTime,m_emReadoutMode,*m_objConfigReader);
    }

    void LambdaSysImpl::InitDetectorSettings()
    {
        LOG_TRACE(__FUNCTION__);

        ReadConfig();

        if(!InitControl())
        {
            LOG_STREAM(__FUNCTION__,ERROR,"detector control init failed");
            exit (EXIT_FAILURE);
        }
        if(!InitRecv())
        {
            LOG_STREAM(__FUNCTION__,ERROR,"detector data receiver init failed");
            exit (EXIT_FAILURE);
        }

        //cout<<m_nThreadNumbers<<"|"<< m_nRawBufferLength<<"|"<<m_nDecodedBufferLength<<endl;

        SetThreshold(m_nThreshold, m_fEnergy);
        SetShutterTime(m_dShutterTime);
        SetTriggerMode(m_shTriggerMode);
        SetNImages(m_lFrameNo);
        SetBurstMode(m_bBurstMode);
        SetCompressionMethod(m_nCompressionMethod,m_nCompressionLevel);
        SetDistortionCorrecttionMethod(m_nDistortionCorrectionMethod);
    }
}
