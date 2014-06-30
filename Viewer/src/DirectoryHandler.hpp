#ifndef DIRECTORY_HANDLER_HPP_
#define DIRECTORY_HANDLER_HPP_

//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================


#include <string>


class DirectoryHandler
{
public:
    DirectoryHandler();

    static void setExePath(const std::string exePath);
    static std::string shareDir()  {return shareDir_;};
    static std::string etcDir()    {return etcDir_;};
    static std::string concatenate(const std::string &path1, const std::string &path2);
    static void findFiles(const std::string &dirPath,const std::string &startsWith);

private:
    static std::string shareDir_;
    static std::string etcDir_;

};




#endif
