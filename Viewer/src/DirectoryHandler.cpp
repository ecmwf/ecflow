//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================


#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include "DirectoryHandler.hpp"
#include "UserMessage.hpp"

std::string DirectoryHandler::shareDir_;
std::string DirectoryHandler::etcDir_;
std::string DirectoryHandler::configDir_;
std::string DirectoryHandler::rcDir_;

DirectoryHandler::DirectoryHandler()
{
}

// -----------------------------------------------------------------------------
// DirectoryHandler::init
// We set all the directory paths containing any of the config files used by the viewer.
// If we tell this class where the executable started from, then it can work out
// where all the other directories are
// -----------------------------------------------------------------------------

void DirectoryHandler::init(std::string exePath)
{
	//Sets paths in the home directory
	if(char *h=getenv("HOME"))
	{
		boost::filesystem::path homeDir(h);
		boost::filesystem::path configDir = homeDir  /= ".ecflowview";
		boost::filesystem::path rcDir = homeDir  /= ".ecflowrc";

		configDir_=configDir.string();
		rcDir_=rcDir.string();

		//Create configDir if if does not exist
		if(!boost::filesystem::exists(configDir))
		{
			try
			{
				boost::filesystem::create_directory(configDir);
			}
			catch(const boost::filesystem::filesystem_error& err)
			{
				UserMessage::message(UserMessage::ERROR, true,
								std::string("Could not create configDir: " + configDir_ + " reason: " + err.what()));
			}
		}
	}


	//Sets paths in the system directory
    boost::filesystem::path path(exePath);
    boost::filesystem::path binDir   = path.parent_path();
    boost::filesystem::path rootDir  = binDir.parent_path();
    boost::filesystem::path shareDir = rootDir  /= "share";
    shareDir /= "ecflow";
    boost::filesystem::path etcDir   = shareDir /= "etc";

    shareDir_ = shareDir.string();
    etcDir_   = etcDir.string();
}


std::string DirectoryHandler::concatenate(const std::string &path1, const std::string &path2)
{
    boost::filesystem::path p1(path1);
    boost::filesystem::path p2(path2);
    boost::filesystem::path result = p1 /= p2;
    return result.string();
}


void DirectoryHandler::findFiles(const std::string &dirPath,const std::string &startsWith)
{
	boost::filesystem::path path(dirPath);

    boost::filesystem::directory_iterator it(path), eod;

    BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod ))
    {
        if(is_regular_file(p) && boost::algorithm::starts_with(p.filename().string(),startsWith))
        {
            std::string filename = p.filename().string();
        }
    }


}
