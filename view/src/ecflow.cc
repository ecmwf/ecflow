//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <map>
// #include <iostream>
// #include <fstream>
// #include <sstream>
#include <boost/lexical_cast.hpp>
#include "File.hpp"
#include "Str.hpp"
#      include <sys/file.h>
#include "ecflow.h"
#include "directory.h"
#include "host.h"

using namespace ecf;

ecf_list *ecf_node_create(char *text)
{
  return new ecf_list(text);
}

/********************/

static std::map<std::string, ecf_map> servers;

int ecf_nick_read(ecf_map::kind kind)
{
  FILE *fp;
#undef  MAXLEN
#define MAXLEN 128
  char buff[MAXLEN];
  char name[MAXLEN];
  char netname[MAXLEN];
  int  prog;
  std::string servpath;

  if (kind == ecf_map::GLOBAL)
    servpath = directory::system();
  else
    servpath = directory::user();
  servpath += "/servers";
  std::cout << "# servers: " << servpath << "\n";

  if( !(fp=fopen(servpath.c_str(),"r")) ) return FALSE;
  while(fgets(buff,MAXLEN,fp)) {
    name[0] = netname[0] = 0;
    sscanf(buff,(char*)"%s %s %d",name,netname,&prog);

    if(name[0] != 0 && name[0] != '#' && 
       servers.find(name) == servers.end())
      servers.insert(std::make_pair
		     (name, ecf_map(name, netname, prog, kind)));
  }

  fclose(fp);
  return 0;
}

int ecf_nick_load() {
  static bool been_here = false;
  if(been_here) return TRUE;  
  else been_here = true;

  servers.insert(std::make_pair
		 ("localhost", 
		  ecf_map("local","localhost", 3141,ecf_map::GLOBAL)));
  ecf_nick_read(ecf_map::GLOBAL);
  ecf_nick_read(ecf_map::USER);

  for (std::map<std::string, ecf_map>::const_iterator 
	 it = servers.begin(); it != servers.end(); ++it) {
    host_maker::make_host(it->first, 
			  it->second.machine(), 
			  it->second.port());
  }  
  return TRUE;
}

int ecf_nick_write()
{
  std::string servpath = directory::user();
  servpath += "/servers";
  FILE *f = fopen(servpath.c_str(), "w");
  if(!f) return FALSE;  
  for (std::map<std::string, ecf_map>::const_iterator 
	 it = servers.begin(); it != servers.end(); ++it) {
    fprintf(f, "%s\n", it->second.print().c_str());
  }
  fclose(f);
  return TRUE;
}

int ecf_nick_origin(const std::string& name)
{
  std::map<std::string, ecf_map>::iterator found = servers.find(name); 
  if (found != servers.end())
    return (int) found->second.from;
  return (int) ecf_map::GLOBAL;
}

int ecf_nick_delete(const std::string& name)
{
  if (servers.find(name) != servers.end())
    servers.erase(servers.find(name)); 
  return TRUE;
}

int ecf_nick_update(const std::string& name, 
		    const std::string& machine, 
		    int port)
{
   if (servers.find(name) != servers.end())
     servers.erase(servers.find(name));
   servers.insert(std::make_pair
		  (name, ecf_map(name, machine, port, ecf_map::USER)));
   ecf_nick_write();
   return TRUE;
}


long ecf_repeat_julian_to_date(long jdate)
{
	long x,y,d,m,e;
	long day,month,year;

	x = 4 * jdate - 6884477;
	y = (x / 146097) * 100;
	e = x % 146097;
	d = e / 4;

	x = 4 * d + 3;
	y = (x / 1461) + y;
	e = x % 1461;
	d = e / 4 + 1;

	x = 5 * d - 3;
	m = x / 153 + 1;
	e = x % 153;
	d = e / 5 + 1;

	if( m < 11 )
		month = m + 2;
	else
		month = m - 10;


	day = d;
	year = y + m / 11;

	return year * 10000 + month * 100 + day;
}

long ecf_repeat_date_to_julian(long ddate)
{
	long  m1,y1,a,b,c,d,j1;

	long month,day,year;

	year = ddate / 10000;
	ddate %= 10000;
	month  = ddate / 100;
	ddate %= 100;
	day = ddate;

	if (month > 2)
	{
		m1 = month - 3;
		y1 = year;
	}
	else
	{
		m1 = month + 9;
		y1 = year - 1;
	}
	a = 146097*(y1/100)/4;
	d = y1 % 100;
	b = 1461*d/4;
	c = (153*m1+2)/5+day+1721119;
	j1 = a+b+c;

	return(j1);
}

char *ecf_string(char *str, char *file, int lineno)
{
  static const char *null_string = "(null string)";
  return str ? str : const_cast<char *>(null_string);
}
static const char* const_ecf_string(const char *str)
{
   static const char *null_string = "(null string)";
   return str ? str : null_string ;
}

#undef MAXLEN
#define MAXLEN          255        /* For temp names, should be big enough */
#include <sys/stat.h>
#include <dirent.h>

ecf_dir *ecf_file_dir(char *path, char *pattern, int fullname)
/**************************************************************************
?  Read the directory and generate the listing
************************************o*************************************/
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
        else {}                        /* Didn't match */
      }
    }
    closedir(dp);
  } else {};

  return dir;
}

bool ecf_map::operator== (const ecf_map& l) const
  {
    return l.name_ == name_
      && l.machine_ == machine_
      && l.port_ == port_;
  }

const std::string ecf_map::print() const {
  std::stringstream i; i << port_;
  return name_ + "\t\t" + machine_ + "\t\t" + i.str();
}
