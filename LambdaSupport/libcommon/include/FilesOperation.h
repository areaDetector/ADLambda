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

#ifndef __LAMBDA_FILES_OPERATION_H__
#define __LAMBDA_FILES_OPERATION_H__

#include "Globals.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    /**
     * @brief file operation
     * This class provides basic file operation(e.g. load configuration) for library
     */
    class FileOperation
    {
      public:
        /**
         * @brief destructor
         */
        virtual ~FileOperation();

        /**
         * @brief open file
         * @param true:binary file;false: text file
         * @return true:OK; false: cannot open
         */
        virtual bool OpenFile(bool) = 0;

        /**
         * @brief check if file is open
         * @return true: open; false: not open
         */
        virtual bool IsOpen() = 0;

        /**
         * @brief close file
         */
        virtual void CloseFile() = 0;

        /**
         * @brief set file path for reading or writing
         * @param strFilePath file path
         */
        virtual void SetFilePath(string strFilePath);

        /**
         * @brief read data from text file
         * @return data content each line in text file is one entry in vector
         */
        virtual vector<string> ReadDataFromFile();

        /**
         * @brief read int binary file
         * @return  data from int binary file
         */
        virtual vector<int> ReadDataFromIntBinaryFile();
        
        /**
         * @brief read binary file
         * @return  data from binary file
         */
        virtual vector<short> ReadDataFromBinaryFile();
        
        /**
         * @brief write data to file
         */
        virtual void WriteDataToFile();
        
        /**
         * @brief check if file exists
         * @param strFileName full file name
         * @return true: exists;false: does not exist
         */
        virtual bool FileExists(const string strFileName);
            
      protected:
        /**
         * @brief contructor
         */
        FileOperation();
            
      protected:
        string m_strFilePath;
        vector<short> m_vShBinValues;
        vector<int> m_vIntBinValues;
        vector<string> m_vStrContent;
    };///end of class FileOperation   

    /**
     * @brief file reader class
     */
    class FileReader : public FileOperation
    {
      public:
        /**
         * @brief constructor
         */
        FileReader();

        /**
         * @brief constructor
         * @param _strFilePath file path
         */
        FileReader(string _strFilePath);

        ///@see FileOperation::OpenFile()
        bool OpenFile(bool bBinary);

        ///@see FileOperation::IsOpen()
        bool IsOpen();

        ///@see FileOperation::CloseFile()
        void CloseFile();
            
        /**
         * @brief destructor
         */
        ~FileReader();
            
        /**
         * @brief read file content
         */
        virtual vector<string> ReadDataFromFile();

        /**
         * @brief read int binary file
         */
        virtual vector<int> ReadDataFromIntBinaryFile();
        
        /**
         * @brief read binary file content
         */
        virtual vector<short> ReadDataFromBinaryFile();
      private:
        /**
         * @brief endian swap for configuration file
         * @param objT data needs to be swapped
         */
        template <class T>
            void EndianSwap(T *objT);
      private:
        ifstream m_ifFileReader;
    };///end of class FileReader
        
    /**
     * @brief file writer class
     */
    class FileWriter : public FileOperation
    {
      public:
        /**
         * @brief constructor
         * @param _strFilePath file path
         */
        FileWriter(string _strFilePath);

        /**
         * @brief destructor
         */
        ~FileWriter();

        ///@see FileOperation::OpenFile()
        bool OpenFile(bool bBinary);

        ///@see FileOperation::IsOpen()
        bool IsOpen();

        ///@see FileOperation::CloseFile()
        void CloseFile();
            
        ///@see FileOperation::WriteDataToFile()
        void WriteDataToFile();

      private:
        ofstream m_ofFileWriter;
    };///end of class FileWriter     
///end of namespace DetCommonNS    
}

#endif
