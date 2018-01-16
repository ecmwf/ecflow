//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#ifndef DIRECTORY_HANDLER_HPP_
#define DIRECTORY_HANDLER_HPP_

#include <vector>

class DirectoryHandler
{
public:
    enum FileType {File, Dir};

    DirectoryHandler();

    static void init(const std::string& exePath);
    static const std::string& exeDir()  {return exeDir_;}
    static const std::string& shareDir()  {return shareDir_;}
    static const std::string& etcDir()    {return etcDir_;}
    static const std::string& configDir()  {return configDir_;}
    static const std::string& rcDir()    {return rcDir_;}
    static const std::string& uiLogFileName() {return uiLogFile_;}
    static const std::string& uiEventLogFileName() {return uiEventLogFile_;}
    static std::string concatenate(const std::string &path1, const std::string &path2);
    static std::string tmpFileName();
    static bool createDir(const std::string& path);

    static void findDirContents(const std::string &dirPath,const std::string &filterStr,
                FileType type, std::vector<std::string>& res);

    static void findFiles(const std::string &dirPath,const std::string &regExpPattern,
    		    std::vector<std::string>& res);

    static void findDirs(const std::string &dirPath,const std::string &regExpPattern,
                std::vector<std::string>& res);

    static bool copyDir(const std::string &srcDir, const std::string &destDir, std::string &errorMessage);
    static bool removeDir(const std::string &dir, std::string &errorMessage);
    static bool renameDir(const std::string &dir, const std::string &newName, std::string &errorMessage);
    static bool copyFile(const std::string &srcFile, std::string &destFile, std::string &errorMessage);
    static bool removeFile(const std::string &file, std::string &errorMessage);
    static bool truncateFile(const std::string &path,int lastLineNum,std::string &errorMessage);
    static bool isFirstStartUp();

private:
    static std::string exeDir_;
    static std::string shareDir_;
    static std::string etcDir_;
    static std::string configDir_;
    static std::string rcDir_;
    static std::string tmpDir_;
    static std::string uiLogFile_;
    static std::string uiEventLogFile_;
};

#endif
