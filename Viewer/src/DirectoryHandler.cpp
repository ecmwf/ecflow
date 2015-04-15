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
                // Gtk-WARNING: Locale not supported by C library.  Using the fallback locale
	        // boost::filesystem::path::imbue(std::locale("C")); 
                // boost::filesystem::path::imbue(std::locale("C.UTF-8")); 
  	        boost::filesystem::path homeDir(h);

		boost::filesystem::path configDir = homeDir;
		configDir /= ".ecflowview";

		boost::filesystem::path rcDir = homeDir;
		rcDir /= ".ecflowrc";

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
    rootDir  /= "share";

    boost::filesystem::path shareDir = rootDir;
    shareDir /= "ecflow";

    boost::filesystem::path etcDir   = shareDir;
    etcDir /= "etc";

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


void DirectoryHandler::findFiles(const std::string &dirPath,const std::string &startsWith,std::vector<std::string>& res)
{
	boost::filesystem::path path(dirPath);

    boost::filesystem::directory_iterator it(path), eod;

    BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod ))
    {
        if(is_regular_file(p) && boost::algorithm::starts_with(p.filename().string(),startsWith))
        {
            std::string filename = p.filename().string();
            res.push_back(filename);
        }
    }
}

/*
ecf_dir *ecf_file_dir(char *path, char *pattern, int fullname)
{
  struct dirent *de;
  DIR           *dp;
  struct stat    st;

  ecf_dir       *cur = NULL;
  ecf_dir       *dir = NULL;

  bool            ok = true;

  if( (dp=opendir(path)) )
  {
    char name[MAXLEN];
    char *s;

    strcpy(name,path);
    s = name + strlen(path);
    *s++ = '/';

    while( ok && (de=readdir(dp)) != NULL )
    {
      if( de->d_ino != 0 )
      {
        strcpy(s,de->d_name);

        if( !pattern || strncmp(de->d_name,pattern,strlen(pattern))==0 )
          if( lstat(name,&st) == 0 )
          {
            if( (cur = new ecf_dir())) // (ecf_dir*) calloc(1,sizeof(ecf_dir))) )
            {
              if(fullname)
              {
                char buff[MAXLEN];
                sprintf(buff,"%s/%s",const_ecf_string(path),const_ecf_string(de->d_name));
                cur->name_ = strdup(buff);
              }
              else
                cur->name_  = strdup(de->d_name);

              cur->mode  = st.st_mode;
              cur->uid   = st.st_uid;
              cur->gid   = st.st_gid;
              cur->size  = st.st_size;
              cur->atime = st.st_atime;
              cur->mtime = st.st_mtime;
              cur->ctime = st.st_ctime;

              ecf_list_add<ecf_dir>(&dir,cur);
            }
            else
              ok = false;
          }
	  else {}
        else {}
      }
    }
    closedir(dp);
  } else {};

  return dir;
}

*/




