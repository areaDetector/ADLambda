#include "Globals.h"
namespace FSDetCoreNS
{
    Enum_log_level USER_LOG_LEVEL = ERROR;

    void InitLogLevel(Enum_log_level level)
    {
        USER_LOG_LEVEL = level;
    }

    /**
     * @brief output debug information
     */
    void OUTPUT(string strFunc, Enum_log_level ELevel, string StrMsg)
    {
        string strLevel = "UNKNOWN";
        switch(ELevel)
        {
            case TRACE:
                strLevel = "TRACE";
                break;
            case DEBUG:
                strLevel = "DEBUG";
                break;
            case INFO:
                strLevel = "INFO";
                break;
            case WARNING:
                strLevel = "WARNING";
                break;
            case ERROR:
                strLevel = "ERROR";
                break;
        }

        if(ELevel >= USER_LOG_LEVEL)
            cout << "["<<strLevel<<"] (" << strFunc << "): " << StrMsg << endl;
    }

    void OUTPUT2(string strFunc)
    {
        /// trace information
        if(TRACE >= USER_LOG_LEVEL)
            cout << "[TRACE] (" << strFunc << "): starts..." << endl;
    }

    void OUTPUT3(string strMsg)
    {
        if(DEBUG >= USER_LOG_LEVEL)
            cout << "[INFO] " << strMsg << endl;
    }

}
