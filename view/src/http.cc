//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <string.h>

#ifndef http_H
#include "http.h"
#endif

#ifndef init_H
#include "init.h"
#endif

#ifndef host_H
#include "host.h"
#endif

#ifndef node_H
#include "node.h"
#endif

#ifndef url_H
#include "url.h"
#endif


http::http(int port,int argc,char** argv):
	server(port)
{
	init::initialize(argc,argv);
}

http::~http()
{
}

static const char *json = getenv("ECFLOW_JSON");

void http::serve(int soc)
{
	url u(soc);

	char what[1024];
	char serv[128];
	char from[1024];
	char suit[128] = {0, };
	strncpy(what,u.what(), 1024);
	
	char *p = strtok(what,"/");
	strncpy(serv,p?p:"", 128);

	p = strtok(0,"/");
	snprintf(suit,128, "%s",p?p:"");

	p = strtok(0,"?");
	snprintf(from,1024, "/%s/%s",suit,p?p:"");
	
	if (json) { // set by environment variable
	    node::is_json = true;
	} else {
	    node::is_json = false;
	}
	if (strlen(suit) > 5) {
	  if (!strncmp(".json", suit+strlen(from) - 5, 5)) {
	    suit[strlen(suit) - 5] = '\0';
	  }
	}
	if (strlen(from) > 5) {
	  if (!strncmp(".json", from+strlen(from) - 5, 5)) {
	    from[strlen(from) - 5] = '\0';
	    node::is_json = true; // set by url name
	  }
	}
	printf("get [%s] [%s] [%s]\n",serv,suit,from);

	// host::login(serv);

	host* ho = host::find(serv);      
	if(ho) { 
	  if (suit[0] != 0) {
	    std::vector<std::string> regist;
	    regist.push_back(suit);
	    ho->suites(SUITES_REG, regist);
	  }
	  ho->login();
	  ho->status();
	}
	node* n = host::find(serv,from);

	u.process(n);

}

int wmain(int argc,char** argv)
{
	char *p = getenv("ECFLOW_HTTP_PORT");
	http s(atol(p),argc,argv);
	s.run();
	return 0;
}
