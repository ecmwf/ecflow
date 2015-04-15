//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogServer.hpp"

#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "setjmp.h"

#ifdef AIX
#include <memory.h>
#endif

//#include "AutoAlarm.hpp"

#define FAIL(a) do { perror(a); exit(1); } while(0)

LogServer::LogServer(std::string host,std::string cport) :
	soc_(-1),
	host_(host),
	port_(cport)
{
	struct hostent *ht = gethostbyname( host.c_str() );

	if (ht == NULL)
	{
		soc_ = -1;
		return;
	}
	connect(host,!cport.empty() ? atoi(cport.c_str()) : 19999);
}

static jmp_buf env;
/* static void (*old_alarm)(int); */

static void catch_alarm(int)
{
	printf("got alarm\n");
	/* longjmp(env,1); */
}

void LogServer::connect(std::string host,int port)
{
	//typedef unsigned long addr_type;
	typedef in_addr_t addr_type;

	//ddr_type none = (addr_type)-1;
	addr_type addr;

	struct sockaddr_in s_in;
	struct hostent *him;

	soc_ = socket(AF_INET, SOCK_STREAM, 0);
	if(soc_ < 0)
	{
		//gui::syserr("Cannot create socket");
		return;
	}

	bzero(&s_in,sizeof(s_in));

	s_in.sin_port = htons(port);
	s_in.sin_family = AF_INET;
	addr = inet_addr(host.c_str());
	s_in.sin_addr.s_addr = addr;

#ifdef SVR4
	if(addr == (in_addr_t) 0xffffffff)
#else
	if(addr == INADDR_NONE)
#endif
	{
		if ((him=gethostbyname(host.c_str()))==NULL)
		{
			//gui::error("Unknown Host %s",host.c_str());
			return;
		}
		s_in.sin_family = him->h_addrtype;
		bcopy(him->h_addr_list[0],&s_in.sin_addr,him->h_length);
	}

	char* timeout = getenv ("ECFLOWVIEW_LOGTIMEOUT");
	int time_out = timeout ? atoi(timeout) : 3;

	struct sigaction sa = { { 0, },  };
	struct sigaction old;
	sa.sa_handler = catch_alarm;
	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGALRM, &sa, &old))
		perror("sigaction");

	::alarm(time_out);
	perror("alarm");

	if(setjmp(env) == 0)
	{
		printf("connect %s\n",host.c_str());
	    if(::connect(soc_,(struct sockaddr*)&s_in,sizeof(s_in)) < 0)
		{
			perror("connect");
			close(soc_);
			soc_ = -1;
		}
	}
	else
	{
		printf("cleanup up\n");
		close(soc_);
		soc_ = -1;
	}
	::alarm(0);
	sigaction(SIGALRM, &old, &sa);
}

LogServer::~LogServer()
{
	close(soc_);
}

VFile_ptr LogServer::getFile(std::string name)
{
	VFile_ptr empty;

    if(soc_ < 0)
    	return empty;

	write(soc_,"get ",4);
	write(soc_,name.c_str(),name.size());
	write(soc_,"\n",1);

	const int size = 64*1204;
	char buf[size];
	unsigned int len = 0;
	int total = 0;

	VFile_ptr out(VFile::create(false));
	FILE *f = fopen(out->path().c_str(),"w");

	if(!f)
	{
	  char buf[2048];
	  sprintf(buf,"Cannot create %s",out->path().c_str());
	  //gui::syserr(buf);
	  return empty;
	}

	while( (len = read(soc_,buf,size)) > 0)
	{
	  if(fwrite(buf,1,len,f) != len)
	    {
	      char buf[2048];
	      sprintf(buf,"Write error on %s",out->path().c_str());
	      //gui::syserr(buf);
	      fclose(f);
	      return empty;
	    }
	  total += len;
	}

	sprintf(buf, "\n# served by %s@%s # telnet %s %s # get %s",
		host_.c_str(), port_.c_str(),
		host_.c_str(), port_.c_str(),
		name.c_str());
	fwrite(buf,1,size,f);

	if(fclose(f))
	{
	  char buf[2048];
	  sprintf(buf,"Write error on %s",out->path().c_str());
	  //gui::syserr(buf);
	  return empty;
	}

	if(total)
		return out;

	return empty;
}

VDir_ptr LogServer::getDir(const char* name)
{
	VDir_ptr empty;

	if(soc_ < 0)
		return empty;

	write(soc_,"list ",5);
	write(soc_,name,strlen(name));
	write(soc_,"\n",1);

	FILE* f = fdopen(soc_,"r");

	char buf[2048];
	std::string strName(name);

	VDir_ptr dir(new VDir(strName));

	while(fgets(buf,sizeof(buf),f))
	{
		int mode,uid,gid;
		unsigned int size;
		unsigned int atime,mtime,ctime;
		char name[1024];

		sscanf(buf,"%d %d %d %u %u %u %u %s",
				&mode,&uid,&gid,
				&size,
				&atime,&mtime,&ctime,
				name);

		dir->addItem(name,size,mtime);
	}

	return dir;
}
