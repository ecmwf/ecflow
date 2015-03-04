
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

static char* debug = getenv("DEBUG");
substitute::substitute(const std::string& name):
  name_(name)
{
}

substitute::~substitute()
{
}

const char* substitute::scan(const char* cmd,node* n)
{
  static char buf[1024];
  int i = 0, j = 0, k= 0, cont = 1;
  char word[1024], edit[1024];
  bool var = false, col = false;
  substitute* s;
  word[0] = 0; edit[0] = 0;

  if (debug) std::cout << "# substituted1:" << cmd << "\n";
  std::string replace (cmd);
  if (replace.find("%") != std::string::npos) {
    if (n->__node__()) {
      if (n->__node__()->type() == NODE_SUPER) {
	ecf_concrete_node<Defs> *ecfn = dynamic_cast<ecf_concrete_node<Defs>*>(n->__node__());
	if (0x0 != ecfn) // ok with a node, NOK with attribute
	  if (const_cast<Defs*>(ecfn->get())) {
	    const_cast<Defs*>(ecfn->get())-> variableSubsitution(replace);
	    strcat(buf, replace.c_str());
	    return buf;
	  }
      } else if (n->__node__()-> get_node()) {
	n->__node__()-> get_node()-> variableSubsitution(replace);
	strcat(buf, replace.c_str());
	if (debug) std::cout << "# substituted2:" << replace << "\n";
	if (debug) std::cout << "# substituted2:" << buf << "\n";
	return buf;
      } 
    }
    return cmd;
  }

  while(*cmd && cont) {
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
	      break;
	    }
	    s = s->next();
	  }
	  
	  buf[i] = 0;
	  strcat(buf,word);
	  i += strlen(word);
	  if (debug) std::cout << "# substituted:" << buf << "-" << word <<"-\n";
	  j = 0;
	  break;
	  
    default:
      if (col) edit[k++] = *cmd;
      else if(var)
	word[j++] = *cmd;
      else
	buf[i++] = *cmd;
      break;
    }
      
    cmd++;
  }
   
  if(k) {
    buf[i] = 0;
    strcat(buf,edit);
    i += strlen(edit);
  } else if(j) {
    buf[i] = 0;
    strcat(buf,word);
    i += strlen(word);
  }
 
  if (debug) std::cout << "# substituted:" << buf << "-" << word << "-" << edit <<"-\n";
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
