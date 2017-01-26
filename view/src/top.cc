//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #20 $ 
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

#include "top.h"
#include "ecflowview.h"
#include "gui.h"
#include "mail.h"            
#include "servers_prefs.h"            
#ifndef tree_H
#include "tree.h"
#endif
#include <Xm/PushB.h>

#ifndef panel_window_H
#include "panel_window.h"
#endif

#ifndef window_H
#include "window.h"
#endif

#ifndef error_H
#include "error.h"
#endif


#ifndef runnable_H
#include "runnable.h"
#endif

#ifndef init_H
#include "init.h"
#endif

#ifndef show_H
#include "show.h"
#endif

#ifndef host_H
#include "host.h"
#endif

#ifndef tip_H
#include "tip.h"
#endif

#ifndef search_H
#include "search.h"
#endif

#ifndef option_H
#include "option.h"
#endif

#ifndef pref_window_H
#include "pref_window.h"
#endif

#ifndef ask_H
#include "ask.h"
#endif


#ifndef error_H
#include "error.h"
#endif


#include "late.h"
#include "restart.h"
#include "aborted.h"

extern "C" {
#include "xec.h"
}

#include <X11/IntrinsicP.h>
#include <Xm/ToggleB.h>

#ifndef globals_H
#include "globals.h"
#endif

#include "Version.hpp"

static option<int> top_width(globals::instance(), "top_width",500);
static option<int> top_height(globals::instance(),"top_height",500);
static option<int> top_xoff(globals::instance(),"top_xoff",0);
static option<int> top_yoff(globals::instance(),"top_yoff",0);

top::top():
	timeout(60)
{
}


top::~top()
{
	Dimension w,h;
	Position x,y;

	XtVaGetValues(this->form_,
		XmNwidth, &w,
		XmNheight,&h,
		XmNx,&x,
		XmNy,&y,
		NULL);

	top_width = w;
	top_height = h;
	top_xoff  = x;
	top_yoff  = y;
}

class initor : public runnable {

	int  argc_;
	char **argv_;

	void run() {
		disable();
		init::initialize(argc_,argv_);
		delete this;
	}

public:
	initor(int argc,char** argv) : argc_(argc),argv_(argv) { enable(); }
};

static void set_show(Widget w, int flag)
{
  CompositeWidget c = CompositeWidget(w);
  for(unsigned i = 0 ; i < c->composite.num_children; i++) {
    Widget p = c->composite.children[i];
    if(XmIsToggleButton(p)) {
      show *s = (show*)xec_GetUserData(p);
      Boolean b = False;
      bool wanted = s->wanted();
      if (flag == show::all) {
	b = True; wanted = true; }
      if ((s->flag() == show::all) || (s->flag() == show::none)) {
	b = False; wanted = false; }
      XmToggleButtonSetState(p,wanted,b);
    }
  }
}

static int xerror(Display *d, XErrorEvent *e)
{
	char buf[1024];
	XGetErrorText(d,e->error_code,buf,sizeof(buf));
	printf("xerror %s\n",buf);
	return 0;
}

void top::create(Display *display, char *app_name, 	
	int app_argc, char **app_argv, char *app_class_name)
{

#if 1
   // XSynchronize should *only* be enabled for debug, it should not be enabled in the production build.
   //   See: [JIRA] (SUP-349) ecflowview performance slow with ecflow 3_1_rc1
   // It can cause refresh issues when displaying synchronisation enabled.
   //	XSynchronize(display,1);
	XSetErrorHandler(xerror);
#endif

#include "xresources.h"		

	XrmDatabase db = XrmGetStringDatabase(xresources);
	XrmSetDatabase(display,db);

	top_shell_c::create(display,app_name,app_argc,app_argv,app_class_name);

	Dimension w,h;
	Position x,y, ac = 0;
	char color[20];
	snprintf(color, 20, "#e5e5e5e5e5e5");

	w = top_width;
	h = top_height;
	x = top_xoff;
	y = top_yoff;

	while (ac < app_argc) {
	  /* accept command line directives for display */
	  if (!strncmp("-geometry=",app_argv[ac],10)) {	  
	    int ww=0, hh=0, xx=0, yy=0;
	    sscanf(app_argv[ac], "-geometry=%dx%d+%d+%d", &ww, &hh, &xx, &yy);
	    fprintf(stdout, "# geometry: %dx%d+%d+%d\n", ww, hh, xx, yy);
	    w=ww; h=hh; x=xx; y=yy;
	  } else if (!strncmp("-b",app_argv[ac],2)) {
	      if (!strncmp("-bg=",app_argv[ac],4)) {
		sscanf(app_argv[ac], "-bg=%s", color);
	      } else if (!strncmp("-background=",app_argv[ac],12)) {
		sscanf(app_argv[ac], "-background=%s", color);
	      } 
		
	      std::string res = "ecFlowview*background: ";
	      res += color;
	      std::cout << "# bg color change: " << res << "\n";
	      db = XrmGetStringDatabase(res.c_str());
	      XrmSetDatabase(display,db);
	  } else if (!strncmp("-rc=",app_argv[ac],4)) {	         
	    char rcdir[1024] = { 0 };
	    sscanf(app_argv[1], "-rc=%s", rcdir);
	    if (rcdir[0] != 0) {
	      std::string var = "ECFLOWRC="; var += rcdir;
	      putenv((char*) var.c_str());
	      fprintf(stdout, "# rcdir: %s\n", rcdir);
	    }
	  }
	  ac++;
	}

	XtVaSetValues(this->form_,
		XmNwidth, w,
		XmNheight,h,
		XmNx, x,
		XmNy, y,
// XmNbackground, (int)pixel(color),
// XtVaTypedArg, XmNbackground, XmRString, color.c_str(), size, // 201403
		NULL);
#if 0
	XtGetApplicationResources(_xd_rootwidget,
				  (XtPointer)&globals,
				  resources,
				  XtNumber(resources),
				  NULL,NULL);
       
	XtGCMask valuemask  = (GCForeground | GCFont);
	XFontStruct* font   = XLoadQueryFont(XtDisplay(_xd_rootwidget),"fixed");

	XGCValues values;
	values.font         = font->fid;
	values.foreground   = globals.black;

	globals.blackGC   = XtGetGC(_xd_rootwidget,valuemask,&values);
	values.foreground = globals.blue;
	globals.blueGC    = XtGetGC(_xd_rootwidget,valuemask,&values);

	for(int j=0;j<STATUS_MAX;j++)
	{
		values.foreground = globals.colors[j];
		globals.colGC[j]  = XtGetGC(_xd_rootwidget,valuemask,&values);
	}
#endif
	set_show(show0_,0); set_show(show1_,0);
	set_show(show2_,0); set_show(show3_,0);

	tip::makeTips(tools_);
	//==========================================

	new initor(app_argc,app_argv);

	// Update and start clock
	run();
	enable();
}

void top::serverCB(Widget,XtPointer)
{
	host::broadcast();
}

static void hostCB(Widget w,XtPointer,XtPointer cb_data)
{
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*)cb_data;
  char* name = XtName(w);
  if(cb->set)
    host::login(name);
  else
    host::logout(name);
}

void top::add_host(const std::string& host)
{
  Widget w;
  if(!(w=XtNameToWidget(servers_menu_,host.c_str())))
    {
      w = XmCreateToggleButton(servers_menu_,
			       (char*)host.c_str(),NULL,0);
      XtAddCallback(w,XmNvalueChangedCallback,hostCB,NULL);
    }
  XtManageChild(w);
  servers_prefs::add_host(host);                       
}

void top::remove_host(const std::string& host)
{
  Widget w = XtNameToWidget(servers_menu_,host.c_str());
  XtUnmanageChild(w);
}

void top::login(const char* host)
{
  Widget w = XtNameToWidget(servers_menu_,host);
  if(w) 
    XmToggleButtonSetState(w,True,False);
  mail::login(host); 
}

void top::logout(const char* host)
{
  Widget w = XtNameToWidget(servers_menu_,host);
  if(w) XmToggleButtonSetState(w,False,False);
  mail::logout(host);   
}

class reset_message : public runnable {
	void run() { gui::clear(); disable(); }
};

void top::clear()
{
	run();
}

void top::message(const char* buf)
{
	static reset_message reset;
 	xec_SetLabel(message_,buf);
    	XmUpdateDisplay(message_);
	reset.enable();
}


Widget top::top_shell()
{
	return xd_rootwidget();
}

Widget top::trees()
{
	return trees_;
}

Widget top::windows()
{
	return windows_menu_;
}

void top::run()
{
	time_t t = time(0);
	char buf[1024];
	// strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M" ,gmtime(&t));
	strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S" ,gmtime(&t));
	xec_SetLabel(message_,buf);
}

void top::watch(Boolean)
{
}

void top::quitCB(Widget,XtPointer)
{
  extent<host>::delete_all(); 
  delete this;
  exit(0);
}

void top::snapshotCB( Widget w, XtPointer p )
{
  char cmd[1024];
  FILE *f = 0;

  gui::message("using SNAPSHOT ; press button \n");
  sprintf(cmd,"${SNAPSHOT:=import} %s\n", snapshotName);
  
  f = popen(cmd,"r");
  if(!f) {
    gui::error("Cannot create snapshot : %s", cmd);
    return;
  }
  else if (!pclose(f)) {
    gui::message("%s # generated\n", snapshotName);
    sprintf(cmd,"${SNAPVISU:=firefox} %s\n", snapshotName);  
    f = popen(cmd,"r");
  } 
  else {
    gui::error("Cannot create snapshot : %s", cmd);
    return;
  }
}

void top::statusCB(Widget,XtPointer data)
{
  XmPushButtonCallbackStruct* cb = (XmPushButtonCallbackStruct*)data;
  host::status((cb->event->xbutton.state & ShiftMask) != 0);
}

void top::windowCB(Widget w,XtPointer data)
{
  XmPushButtonCallbackStruct *cb = (XmPushButtonCallbackStruct *) data;
  
  panel_window::new_window
    (selection::current_node(),
     XtName(w), 
     (cb->event->xbutton.state & ShiftMask) != 0,
     (cb->event->xbutton.state & ShiftMask) != 0
     );
}

void top::showCB(Widget w,XtPointer data)
{
  XmRowColumnCallbackStruct *cb = (XmRowColumnCallbackStruct *) data;
  show *s = (show*)xec_GetUserData(cb->widget);
  if(s && XmIsToggleButton(cb->widget)) {
    int flag = s->flag();
    if (((flag == show::none) | (flag == show::all))) {
       s->off();
       set_show(show0_, flag); set_show(show1_, flag); 
       set_show(show2_, flag); set_show(show3_, flag);   
       XmToggleButtonSetState(cb->widget,false,False);   
    } else if (XmToggleButtonGetState(cb->widget))
      s->on();
    else
      s->off();     
    tree::update_all(true);
  }
}

void top::chatCB(Widget w,XtPointer)
{
  host::chat();
}

void top::windowsCB(Widget,XtPointer data)   
{
  XmRowColumnCallbackStruct *cb = (XmRowColumnCallbackStruct *) data;
  window *w = (window*) xec_GetUserData (cb->widget);
  w->raise();
}

void top::searchCB(Widget w,XtPointer)
{
	search::show();
}

void top::releaseCB(Widget w,XtPointer)
{
   std::string version = "version " + ecf::Version::raw();
   gui::message(version.c_str());
  char cmd[1024];
  snprintf(cmd, 1024, "${ECFLOWVIEW_HELP:=firefox --new-tab %s}\n", urlRef);
  // snprintf(cmd, 1024, "%s %s\n", browserName, urlRef);
  std::cerr << "#INF: " << cmd;
  if (system(cmd)) { std::cerr << "#ERR release\n"; }
  // execlp(browserName, urlRef, NULL);
  sleep (1);
}

void top::helpCB(Widget w,XtPointer)
{
   const char *link="http://intra.ecmwf.int/metapps/manuals/ecflow/index.html";
   // const char *jira="http://software.ecmwf.int/issues/browse/ECFLOW";
   // const char *link="http://wedit.ecmwf.int/publications/manuals/ecFlow";
   char cmd[1024];

   gui::message("ecFlowView help (ECFLOWVIEW_HELP); press button\n");
   snprintf(cmd, 1024, "${ECFLOWVIEW_HELP:=firefox --new-tab %s}\n", link);
   std::cerr << "#INF: " << cmd;

   if (1) { 
     if (system(cmd)) { std::cerr << "#ERR system\n"; }
   } else {
      FILE* f = popen(cmd,"r");
      if(!f) {
         gui::error("Cannot access : %s", link);
         return;
      } else if (!pclose(f)) {
      } else {
         gui::error("Cannot access : %s", link);
         return;
      }
   }
}

void top::prefCB(Widget w,XtPointer)
{
  pref_window::show();
}

void top::loginCB(Widget w,XtPointer)
{
  static str s;
  if(ask::show(s,"Login to (host [port]): ")) {
    char h[80] = { 0 };
    int  n = 3141;
    sscanf(s.c_str(),"%s %d",h,&n);
    if (h[0] != 0)
      host::login(h,n);
  }
}

void top::error(const char* msg)
{
  error::show(msg);
}
