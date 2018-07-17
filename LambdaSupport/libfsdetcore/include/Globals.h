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
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <map>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ostream>
#include <pthread.h>
#include <queue>
#include <set>
#include <sstream>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include "Version.h"

namespace FSDetCoreNS
{
    /* define fixed width integer type */
    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    typedef size_t szt;
    typedef ssize_t sszt;
    typedef unsigned char uchar;

    const int8 int8min = INT8_MIN;
    const int8 int8max = INT8_MAX;
    const uint8 uint8max = UINT8_MAX;

    const int16 int16min = INT16_MIN;
    const int16 int16max = INT16_MAX;
    const uint16 uint16max = UINT16_MAX;

    const int32 int32min = INT32_MIN;
    const int32 int32max = INT32_MAX;
    const uint32 uint32max = UINT32_MAX;

    const int64 int64min = INT64_MIN;
    const int64 int64max = INT64_MAX;
    const uint64 uint64max = UINT64_MAX;


    /// udp package size
    const uint16 UDP_PACKET_SIZE_NORMAL = 8934;

    const uint16 UDP_EXTRA_BYTES = 6;    // bytes

    const uint16 CHAR_BUFFER_SIZE = 9000;

    using namespace std;

    /**
     * @brief net work protocol
     */
    enum Enum_protocol {
        TCP, /**< enum value 0 */
        UDP
    };

    /**
     * @brief file mode enum
     * used to indicate the mode for opening file
     */
    enum Enum_FileMode {
        READ, /**< enum value 0 */
        WRITE
    };

    /**
     * @brief priority enum
     * used for indicating task priority
     */
    enum Enum_priority {
        LOW, /**< enum value 0 */
        NORMAL,
        HIGH
    };

    /**
     * @brief log_level enum
     * mainly used for indicating the log level
     */
    enum Enum_log_level {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    /**
     * @brief detector state enum
     * this contains additional states compared to Tango,
     * but ON DISABLE and FAULT are the same and all other define states can be
     * evaluated as RUNNING
     */
    enum Enum_detector_state {
        ON, /** < enum value 0 */
        DISABLE,
        BUSY, /** Indicates performing a non-imaging task e.g. loading config */
        FAULT,
        RECEIVING_IMAGES,  /** Taking images, still receiving images from detector */
        PROCESSING_IMAGES, /** All images received, images still in buffer awaiting readout */
        FINISHED           /** All images received, buffer empty.
                            *  Returns to ON state after StopAcquisition called.
                            *  This allows the Tango server to keep out of the ON state
                            *  until all Tango-side tasks like file writing are
                            * done. */
    };

    void InitLogLevel(Enum_log_level level);
    /**
     * @brief output debug information
     */
    void OUTPUT(string strFunc, Enum_log_level ELevel, string StrMsg);

    void OUTPUT2(string strFunc);

    void OUTPUT3(string strMsg);

    /**
     * @brief marco definition for basic system info printout
     */
    #define LOG_STREAM(FuncMsg, ErrLevel, ErrMsg) OUTPUT(FuncMsg, ErrLevel, ErrMsg)

    #define LOG_TRACE(FuncMsg) OUTPUT2(FuncMsg)

    #define LOG_INFOS(InfoMsg) OUTPUT3(InfoMsg)
}
