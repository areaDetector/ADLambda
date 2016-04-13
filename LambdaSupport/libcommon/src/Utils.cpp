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

#include "Utils.h"

///namespace DetCommonNS
namespace DetCommonNS
{
    //////////////////////////////////////////////////
    /// StringUtils
    //////////////////////////////////////////////////
    void StringUtils::StrSplit(const string strOriginal, const string strDelimiters , vector<string>& vStrArray)
    {
        LOG_TRACE(__FUNCTION__);
        
        // Skip delimiters at beginning.
        string::size_type lastPos = strOriginal.find_first_not_of(strDelimiters, 0);

        // Find first "non-delimiter".
        string::size_type pos     = strOriginal.find_first_of(strDelimiters, lastPos);
        
        while (string::npos != pos || string::npos != lastPos)
        {
            // Found a token, add it to the vector.
            vStrArray.push_back(strOriginal.substr(lastPos, pos - lastPos));

            // Skip delimiters.  Note the "not_of"
            lastPos = strOriginal.find_first_not_of(strDelimiters, pos);

            // Find next "non-delimiter"
            pos = strOriginal.find_first_of(strDelimiters, lastPos);
        }
    }
    
    void StringUtils::FindAndReplace(string& strSrc, string const& strFind,string const& strReplace)
    {
        LOG_TRACE(__FUNCTION__);
               
        for(std::string::size_type i = 0; (i = strSrc.find(strFind, i)) != std::string::npos;)
        {
            strSrc.replace(i, strFind.length(), strReplace);

            i += strReplace.length() - strFind.length() + 1;
        }
    }

    void StringUtils::RemoveChar(string& strVal,char chDelimiter)
    {
        LOG_TRACE(__FUNCTION__);
        
        int i=0;
        vector<char> vChar;
        while(strVal[i]!='\0')
        {
            if(strVal[i]!=chDelimiter)
                vChar.push_back(strVal[i]);
            i++;
        }

        string strTmp(vChar.begin(),vChar.end());
        strVal = strTmp;
    }

    char StringUtils::ReverseChar(char charSrc)
    {
        LOG_TRACE(__FUNCTION__);

	char charOutput = '\0';
	int bits = 8;

	for(int i = 0; i < bits; i++)
	{
	    charOutput <<=1; // left shift output
	    charOutput |= (charSrc & 0x1); //get unit bit
	    charSrc >>= 1; // right shift input
	}

	return charOutput;

    }

    
    //////////////////////////////////////////////////
    /// FileUtils
    //////////////////////////////////////////////////
    vector<string> FileUtils::GetFileList(string strPath)
    {
        LOG_TRACE(__FUNCTION__);

        vector<string> vStrFileList;
        struct dirent *objDir=NULL;
        DIR *objD=NULL;
        
        objD=opendir(strPath.c_str());
        if(objD == NULL)
        {
            LOG_STREAM(__FUNCTION__,ERROR,"cannot open directory!");
            return vStrFileList;
        }
        
        while(objDir = readdir(objD))
        {
            if(string(objDir->d_name) != "." && string(objDir->d_name) != "..")
                vStrFileList.push_back(string(objDir->d_name));
        }
        closedir(objD);
        return vStrFileList;
    
    }
    

    //////////////////////////////////////////////////
    /// DataConvert 
    //////////////////////////////////////////////////
    void DataConvert::IntToUChar(int nVal, vector<unsigned char>& vUChar)
    {
        LOG_TRACE(__FUNCTION__);
        
        vUChar[0] = (nVal & 0xFF000000) >> 24;
        vUChar[1] = (nVal & 0x00FF0000) >> 16;
        vUChar[2] = (nVal & 0x0000FF00) >> 8;
        vUChar[3] = (nVal & 0x000000FF);
    }
    
}///end of namespace DetCommonNS

