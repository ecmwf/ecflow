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

#include <unistd.h>
#include "host.h"
#include "edit.h"
#include "node.h"
#include "globals.h"
#include "input.h"
#include "error.h"
#include "lister.h"
#include "tmp_file.h"

#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <X11/IntrinsicP.h>
#include <vector>

static const char* micro="%";
const char* sStart = "comment - ecf user variables";
const char* sEnd   = "end - ecf user variables";

extern "C" {
#include "xec.h"
}

edit::edit(panel_window& w):
	panel(w),
	text_window(false),
	loading_(False),
	preproc_(False),
	tmp_(0), kStart(0x0), kEnd(0x0)
{
  if (kStart==NULL) 
    kStart=(char*)calloc(1024, sizeof(char*));
  if (kEnd==NULL)   
    kEnd=(char*)calloc(1024, sizeof(char*));
}

edit::~edit()
{
  if(tmp_) XtFree(tmp_);
  if (kStart) free(kStart);
  if (kEnd) free(kEnd);
}

void edit::create (Widget parent, char *widget_name )
{
	edit_form_c::create(parent,widget_name);
        XmToggleButtonSetState(alias_, globals::get_resource("send_as_alias", 0), FALSE);
}

void edit::clear()
{
	loading_ = True;
	XmTextSetString(text_,(char*)"");
	loading_ = False;
}

void edit::show(node& n)
{
  loading_ = True;
  XmTextSetString(text_,(char*)"");

  // tmp_file v(tmpnam(0), true); FILE *f = fopen(v.c_str(),"w");
  char tmpname[] = "/tmp/xecfXXXXXX";
  int  fid = mkstemp(tmpname);
  FILE *f = fdopen(fid, "w");

  if(!f) {
    gui::syserr(tmpname);
    return;
  }
  
  std::list<Variable> vl; // FILL handle vl
  tmp_file tmp(NULL);
  tmp = n.serv().edit(n, vl, preproc_);
  
  if(fclose(f)) {
    gui::syserr(tmpname);
    return;    
  }

  try {
  xec_LoadText(text_, tmpname, True);
  xec_LoadText(text_, tmp.c_str(), True);
  
  XmTextSetInsertionPosition(text_,0);
  XmTextShowPosition(text_, 0);
  } catch (...) { std::cerr << "# WAR: cannot load " << tmpname << "\n";}
  loading_ = False;
}

void edit::changed(node&)
{
}

Boolean edit::enabled(node& n)
{
	return n.type() == NODE_TASK;
}

void edit::changedCB(Widget,XtPointer data)
{
	if(!loading_) freeze();
}

void edit::preprocCB(Widget,XtPointer data)
{
	preproc_ = XmToggleButtonGetState(preprocess_);
	if(get_node())
		show(*get_node());
	else
		clear();
}

static char* strip(char* n)
{
  int l = strlen(n) - 1;
  while(l >= 0 && n[l] == ' ')
    n[l--] = 0;
  
  char* p = n;
  while(*p && *p == ' ') p++;
  
  return p;
}

void edit::submitCB(Widget,XtPointer)
{
  bool alias = XmToggleButtonGetState(alias_);
  bool run   = true;
  char line[4096];
  node *nd = get_node();
  
  if(nd) {
    tmp_file t(tmpnam(0), true);
    if(xec_SaveText(text_,(char*)t.c_str())) {
      gui::syserr(t.c_str());
      return;
    }
    
    NameValueVec var; 
    FILE *f = fopen(t.c_str(),"r");
    if(!f) {
      gui::syserr(t.c_str());
      return;
    }
	
    const std::string& mv = nd->__node__() ? 
      nd->variable("ECF_MICRO") : nd->variable("SMSMICRO");
    const char * mic = (mv.size() == 1) ? mv.c_str() : micro;
    sprintf(kStart, "%s%s", mic, sStart);
    sprintf(kEnd,   "%s%s", mic, sEnd);	  
    
    bool isvars = false;
    while(fgets(line,sizeof(line),f)) {
      line[strlen(line)-1] = 0;
      
      if(isvars) {
	char* p = line;
	while(*p && *p != '=') p++;
	if(*p == '=') {
	  *p = 0;
	  
	  char n[1024];
	  char v[1024];
	  
	  strcpy(n,line);
	  strcpy(v,p+1); 
	  
	  var.push_back(std::make_pair(strip(n), strip(v))); 
	}
      }
            
      if (strcmp(line,kStart) == 0)
	isvars = true;			  
      if(strcmp(line,kEnd) == 0)
	break;      
    }
    
    if(var.empty()) {
      gui::message("No user variables!");
      // return;
    }
    
    get_node()->serv().send(*get_node(),alias,run,var,t.c_str());
    
  } else 
    clear();

  if (alias != globals::get_resource("send_as_alias", 0))
    globals::set_resource("send_as_alias", alias);
  
  submit();
}

void edit::externalCB(Widget,XtPointer)
{
	if(tmp_) XtFree(tmp_);
	tmp_ = XtNewString(tmpnam(0));

	if(xec_SaveText(text_,tmp_))
	{
		gui::syserr(tmp_);
		return;
	}

	char cmd[1024];
	const char* xedit = getenv("XEDITOR");
	if (xedit)
	  sprintf(cmd,"${XEDITOR:=xterm -e vi} %s",tmp_);
	else
	  sprintf(cmd,"xterm -e ${EDITOR:=vi} %s",tmp_);

	FILE *f = popen(cmd,"r");
	if(!f) {
		gui::syserr(cmd);
		return;
	}
	XtSetSensitive(text_,False);
	XtSetSensitive(tools_,False);
	XtSetSensitive(tools2_,False);

	start(f);
}

void edit::ready(const char* line)
{
	gui::error("%s",line);
}

void edit::done(FILE* f)
{
  stop();
  
  if(pclose(f)) {
    gui::error("External editor returns error");
    return;
  }
  
  if(xec_LoadText(text_,tmp_,False))
    gui::syserr(tmp_);
  
  unlink(tmp_);
  
  XtSetSensitive(text_,True);
  XtSetSensitive(tools_,True);
  XtSetSensitive(tools2_,True);
}

// static panel_maker<edit> maker(PANEL_EDIT_TASK);
