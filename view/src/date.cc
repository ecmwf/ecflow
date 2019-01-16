//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
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

#include "date.h"
#include "ecf_node.h"

date_node::date_node(host& h,ecf_node* n) : node(h,n) {
  if (!n) return;
  full_name_ = parent()->full_name();
  full_name_ += ":";
  if (n) full_name_ += n->toString();
}

#ifdef BRIDGE
static char *week[] = {
	(char*)"sunday", (char*)"monday",(char*)"tuesday",(char*)"wednesday",(char*)"thursday",
	(char*)"friday",(char*)"saturday"};

date_node::date_node(host& h,sms_node* n, char b) 
  : node(h,n,b) 
{}
#endif

char* date_node::string(char *s)
{
  char buf[1024];
  s[0] = 0;
  if (owner_)
    snprintf(buf, 1024, "%s", owner_->toString().c_str());
#ifdef BRIDGE
  if (tree_) {
	sms_date *d = (sms_date*)tree_;

	char buf[1024];
	s[0] = 0;

	if(d->weekdays)
	{
		int flg = 0;
		for( int i=0 ; i<DAY_MAX ; i++ )
			if( d->weekdays & ( 1<<i ) )
			{
				if(flg) { 
					strcat(s," ");
				}
				strcat(s,week[i]);
				flg++;
			}
	} else {
			sprintf(buf,d->day<0?"*":"%02d",d->day);
			strcat(s,buf);
			strcat(s,".");

			sprintf(buf,d->month<0?"*":"%02d",d->month);
			strcat(s,buf);
			strcat(s,".");

			sprintf(buf,d->year<0?"*":"%02d",d->year+1900);
			strcat(s,buf);
	}
	return s;
  }
  else 
#endif
  strcat(s,buf);
  return s; 
}

xmstring date_node::make_label_tree()
{
  char buf[1024];
  return xmstring(string(buf));
}

void date_node::perlify(FILE* f) 
{
	char buf[1034];
	perl_member(f,"value",string(buf));
}

node* date_node::graph_node()
{
  return &dummy_node::get(full_name());
}

const std::string& date_node::name() const 
{ 
  static std::string date_ = "date";
#ifdef BRIDGE
  if (tree_) return date_;
#endif
  if (owner_) date_ = owner_->name(); 
  return date_;
}
