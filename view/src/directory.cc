#ifndef directory_H
#include "directory.h"
#endif

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ecflowview.h"

const char* directory::user()
{
	static char x[1024] = {0};
	if(x[0] == 0)
	{
	  const char * rcdir = getenv("ECFLOWRC");
	  if (rcdir) 
	    sprintf(x,"%s",rcdir);
	  else
	    sprintf(x,"%s/.%s",getenv("HOME"),"ecflowrc");
	  mkdir(x,0755);
	  fprintf(stdout, "# rcdir: %s\n", x);
	}
	return x;
}

const char* directory::system()
{
  static char x[1024] = {0};
  if(x[0] == 0) {
    if(getenv("ECFLOWVIEW_HOME"))
      strcpy(x,getenv("ECFLOWVIEW_HOME"));
    else	
      // strcpy(x,"/usr/local/lib/ecflowview");
      // strcpy(x,"/usr/local/apps/sms/lib/ecflow");
      strcpy(x,"/usr/local/apps/ecflow/current/lib");
  }
  return x;
}


FILE* directory::open(const char* name,const char *mode)
{
	FILE* f = 0;
	char buf[1024];

	sprintf(buf,"%s/%s",user(),name);
	f = fopen(buf,mode);
	if(f || *mode != 'r') return f;

	sprintf(buf,"%s/%s",system(),name);
	return fopen(buf,mode);		
}
