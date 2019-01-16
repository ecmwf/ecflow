//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <string>
#include "time_node.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif

#ifdef BRIDGE

static char *week[] = {
  (char*)"sunday",(char*) "monday",(char*)"tueday",
  (char*)"wednesday",(char*)"thursday", (char*)"friday",
  (char*)"saturday"};

static char *month[] = { 
  (char*)"january", (char*)"february", (char*)"march", 
  (char*)"april", (char*)"may", (char*)"june",
  (char*)"july", (char*)"august", (char*)"september", 
  (char*)"october", (char*)"november", (char*)"december",
};

static char* ext[] = {
  (char*)"st",(char*)"nd",(char*)"rd",(char*)"th",(char*)"th",
  (char*)"th",(char*)"th",(char*)"th",(char*)"th",(char*)"th",
  (char*)"th",(char*)"th",(char*)"th",(char*)"th",(char*)"th",
  (char*)"th",(char*)"th",(char*)"th",(char*)"th",(char*)"th",
  (char*)"st",(char*)"nd",(char*)"rd",(char*)"th",(char*)"th",
  (char*)"th",(char*)"th",(char*)"th",(char*)"th",(char*)"th",
  (char*)"st", 
};

time_node::time_node(host& h,sms_node* n, char b) 
  : node(h,n,b)
  , time_("time")
{
}
#endif

time_node::time_node(host& h,ecf_node* n) 
  : node(h,n)
  , time_("time")
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  if (owner_) full_name_ += owner_->toString();
}

char* time_node::string(char* s)
{
  s[0] = 0; 
#ifdef BRIDGE
  if (tree_) {
  sms_time *t = (sms_time*)tree_;
	
	char buf[1024];
	s[0] = 0;

	sprintf(buf,t->hour[0]<0?"*":"%02d",t->hour[0]);
	strcat(s,buf);

	strcat(s,":");

	sprintf(buf,t->minute[0]<0?"*":"%02d",t->minute[0]);
	strcat(s,buf);

	if(t->repeat)
	{
		sprintf(buf," to %02d:%02d",t->hour[1],t->minute[1]);
		strcat(s,buf);

		sprintf(buf," by %02d:%02d",t->hour[2],t->minute[2]);
		strcat(s,buf);
	}


	if(t->iscron)
	{
		long w = t->weekdays;
		int n = 0;
		while(w)
		{
			if(w%2) 
			{
				sprintf(buf," %s",week[n]);
				strcat(s,buf);
			}
			w >>= 1;
			n++;
		}

		w = t->monthdays;
		n = 0;
		while(w)
		{
			if(w%2)
			{
				sprintf(buf," %d%s",n+1,ext[n]);
				strcat(s,buf);
			}
			n++;
			w >>= 1;
		}

		w = t->months;
		n = 0;
		while(w)
		{
			if(w%2) 
			{
				sprintf(buf," %s",month[n]);
				strcat(s,buf);
			}
			w >>= 1;
			n++;
		}
	}
	return s;
}
#endif
  if (owner_) { sprintf(s, owner_->toString().c_str()); return s; }
  return s;
}

xmstring time_node::make_label_tree()
{
  char s[1024];
#ifdef BRIDGE
  sms_time *t = (sms_time*)tree_;
  if (t && t->iscron)
    {
      xmstring c("cron: ","bold");
      xmstring x(this->string(s));
      return c + x;
    }
#endif 
  return xmstring(this->string(s));
}

void time_node::perlify(FILE* f) 
{
	char buf[1034];
	perl_member(f,"value",string(buf));
}

node* time_node::graph_node()
{
  return &dummy_node::get(full_name());
}
