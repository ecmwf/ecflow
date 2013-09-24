
#ifndef substitute_H
#include "substitute.h"
#endif

#ifndef node_H
#include "node.h"
#endif
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

#include <strings.h>

#include <Xm/PushB.h>

extern "C" {
#include "xec.h"
}

// substitute::substitute(const char* name): name_(name) {}

substitute::substitute(const std::string name):
  name_(name)
{
}

substitute::~substitute()
{
}

const char* substitute::scan(const char* cmd,node* n)
{
  static char buf[1024];
  int i = 0;
  char word[1024];
  int j = 0;
  bool var = false;
  substitute* s;
  
  while(*cmd) {
    switch(*cmd) {
    case '<':
	  var       = true;
	  j         = 0;
	  word[j++] = '<';
	  break;
	  
    case '>':	  
	  var       = false;
	  word[j++] = '>';
	  word[j]   = 0;
	  
	  s = first();
	  while(s) {
	    if(s->name_ == word) {
	      strcpy(word,s->eval(n).c_str());
	      // std::cout << "# substituted\n";
	      break;
	    }
	    s = s->next();
	  }
	  
	  buf[i] = 0;
	  strcat(buf,word);
	  i += strlen(word);
	  // std::cout << "# substituted:" << buf << "-" << word <<"-\n";
	  j = 0;
	  break;
	  
    default:
	  if(var)
	    word[j++] = *cmd;
	  else
	    buf[i++] = *cmd;
	  break;
    }
      
    cmd++;
  }
  
  if(j) {
    buf[i] = 0;
    strcat(buf,word);
    i += strlen(word);
  }
  
  buf[i] = 0;
  return buf;
}

void substitute::fill(Widget w)
{
  substitute* s = first();
  while(s) {
    XtManageChild(XmCreatePushButton(w,(char*)s->name_.c_str(),0,0));
    s = s->next();
  }
}

IMP(substitute)
