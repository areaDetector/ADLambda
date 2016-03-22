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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <iostream>
#include <memory>
#include <string.h>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <pthread.h>
#include <sys/time.h>
#include <fstream>
#include <ostream>
#include <sstream>
#include <errno.h>
#include <limits.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include <boost/pool/pool_alloc.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

///namespace DetCommonNS
namespace DetCommonNS
{

    ///udp package size
    const int UDP_PACKET_SIZE_NORMAL = 8934;

    const int UDP_EXTRA_BYTES = 6;//bytes
     
    const int CHAR_BUFFER_SIZE = 9000;
    
    using namespace std;

    /**
     * @brief net work protocol
     */
    enum Enum_protocol
    {
        TCP,                    /**< enum value 0 */
        UDP
    };
    
    /**
     * @brief file mode enum
     * used to indicate the mode for opening file
     */
    enum Enum_FileMode
    {
        READ,                   /**< enum value 0 */
        WRITE
    };    
    
    /**
     * @brief priority enum
     * used for indicating task priority
     */
    enum Enum_priority
    {
        LOW,                    /**< enum value 0 */
        NORMAL,
        HIGH
    };
        
    /**
     * @brief log_level enum
     * mainly used for indicating the log level
     */
    enum Enum_log_level
    {            
        DEBUG,                  /**< enum value 0 */
        WARNING,
        ERROR
    };

    /**
     * @brief detector state enum
     * this contains additional states compared to Tango, but ON DISABLE and FAULT are the same and all other define states can be evaluated as RUNNING
     */
    enum Enum_detector_state
    {
        ON,                     /**< enum value 0 */
        DISABLE,
        BUSY,                   /** Indicates performing a non-imaging task e.g. loading config */
        FAULT,
        RECEIVING_IMAGES,       /** Taking images, still receiving images from detector */
        PROCESSING_IMAGES,      /** All images received, images still in buffer awaiting readout */
        FINISHED                /** All images received, buffer empty. Returns to ON state after StopAcquisition called.
                                 *  This allows the Tango server to keep out of the ON state until all Tango-side tasks like file writing are done. */
    };
  
    /**
     * @brief output debug information
     */
    static void OUTPUT(string strFunc,Enum_log_level ELevel, string StrMsg)
    {
        ///debug information
        #ifdef LDEBUG
        cout<<"==DEBUG=="<<strFunc<<"(): "<<ELevel<<" "<<StrMsg<<endl;
        #endif
    }
    
    static void OUTPUT2(string strFunc)
    {
        ///trace information
        #ifdef LTRACE
        cout<<"==TRACE=="<<strFunc<<"(): starts..."<<endl;
        #endif
    }

    static void OUTPUT3(string strMsg)
    {
        ///information
        #ifdef LINFO
        cout<<"==INFO=="<<strMsg<<endl;
        #endif
    }
    
    /**
     * @brief marco definition for basic system info printout
     */
    #define LOG_STREAM(FuncMsg,ErrLevel, ErrMsg)    \
        OUTPUT(FuncMsg,ErrLevel,ErrMsg)
    
    #define LOG_TRACE(FuncMsg)                  \
        OUTPUT2(FuncMsg)

    #define LOG_INFOS(InfoMsg)                  \
        OUTPUT3(InfoMsg)


}///end of namespace DetCommonNS

#endif
