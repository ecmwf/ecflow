//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

#ifndef server_H
#include "server.h"
#endif

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
#include <string.h>


#ifdef AIX
#include <memory.h>
#endif

#define FAIL(a) do { perror(a); exit(1); } while(0)

server::server(int port):
	soc_(-1)
{
	int flg;
	struct sockaddr_in sin;

#ifdef SO_LINGER
	struct  linger ling;
#endif

	soc_ = socket(AF_INET, SOCK_STREAM, 0);

	if (soc_ < 0)
		FAIL("socket");

	flg = 1 ;
	if(setsockopt(soc_, SOL_SOCKET, SO_REUSEADDR, &flg, sizeof(flg))<0)
		FAIL("setsockopt SO_REUSEADDR");

	flg = 1 ;
	if(setsockopt(soc_, SOL_SOCKET, SO_KEEPALIVE, &flg, sizeof(flg))<0)
		FAIL("setsockopt SO_KEEPALIVE");

#ifdef SO_REUSEPORT
	flg = 1 ;
	if(setsockopt(soc_, SOL_SOCKET, SO_REUSEPORT, &flg, sizeof(flg))<0)
		FAIL("setsockopt SO_REUSEPORT");
#endif

#ifdef SO_LINGER
	ling.l_onoff = 0;
	ling.l_linger = 0;
	if(setsockopt(soc_, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling))<0)
		FAIL("setsockopt SO_LINGER");
#else
#ifdef SO_DONTLINGER
	if(setsockopt(soc_, SOL_SOCKET, SO_DONTLINGER, NULL, 0)<0)
		FAIL("setsockopt SO_DONTLINGER");
#endif
#endif


	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_port        = htons(port);
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	while (bind(soc_, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) == -1)
	{
		FAIL("bind");
		sleep(5);
	}

	/* socket_buffers(s); */

	if(listen(soc_, 5)==-1)
	{
		close(soc_);
		FAIL("listen");
	}

	signal(SIGPIPE,SIG_IGN);
}

server::~server()
{
	close(soc_);
}

void server::run()
{
	struct sockaddr_in from;
//#ifdef AIX
//	unsigned long fromlen;
//#else
	socklen_t fromlen; // int fromlen;
//#endif
	int cnt = 0;
	int clients = 0;

	/* Start real server */

	if(soc_ < 0) FAIL("Exiting server");

	/* Dont't get killed by pipes */
	signal(SIGPIPE,SIG_IGN);

	/* Ignore hang up */
	signal(SIGHUP,SIG_IGN);

	for(;;)
	{
	   int snew;
		fromlen = sizeof(from);
		if((snew = accept(soc_, (struct sockaddr*)&from, &fromlen))<0)
		{
			if(errno != EINTR)
				/* Interrupted system call : got on SIGCHLD signals */
				FAIL("accept");
		}
		else
		{

			if(from.sin_family != AF_INET)
			{
				FAIL("connection is not from internet");
				close(snew);
				continue;
			}

			fflush(0);
			sighold(SIGCHLD);

			if(!fork_)
			{
				serve(snew);
				close(snew);
			}
			else switch(fork())
			{
			case 0:
				close(soc_);
				serve(snew);
				exit(0);
				break;

			case -1:
				FAIL("Cannot fork");
				close(snew);
				break;

			default:
				cnt++;
				clients++;
				close(snew);
				break;
			}
			sigrelse(SIGCHLD);
		}
	}
}
