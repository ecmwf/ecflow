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

#include "collector.h"
#include "gui.h"
#include "node.h"
#include "host.h"
#include "substitute.h"
#include "directory.h"
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/Command.h>
#include <Xm/Xm.h>
extern "C" {
#include "xec.h"
}

collector::collector()
{

  const int commands_nb = 22;          
  static char* commands[commands_nb] = {
    (char *) "ecflow_client --zombie_fob=<full_name>" ,
    (char *) "ecflow_client --zombie_fail=<full_name>" ,
    (char *) "ecflow_client --zombie_adopt=<full_name>" ,
    (char *) "ecflow_client --zombie_block=<full_name>" ,
    (char *) "ecflow_client --alter clear_flag zombie=<full_name>" ,
    (char *) "###",
    (char *) "ecflow_client --suspend=<full_name>" ,
    (char *) "ecflow_client --resume=<full_name>" ,
    (char *) "ecflow_client --kill=<full_name>" ,
    (char *) "ecflow_client --run=<full_name>" ,
    (char *) "###",
    (char *) "ecflow_client --delete=force yes <full_name>",
    (char *) "###",
    (char *) "ecflow_client --begin=<node_name>",
    (char *) "###",
    (char *) "ecflow_client --requeue=<full_name>",
    (char *) "ecflow_client --alter=change defstatus queued <full_name>",
    (char *) "ecflow_client --alter=change defstatus complete <full_name>",
    (char *) "ecflow_client --force=complete <full_name>",
    (char *) "ecflow_client --force=aborted  <full_name>",
    (char *) "###",
    (char *) "sh python %PYDEF:0% %SUITE% <full_name> %ECF_NODE% # aka replace",
    };

	create(gui::top());	
	set_menu("Collector");
	substitute::fill(blocks_);
	XtManageChild(XmCreateSeparator(blocks_,"-",0,0));
	update();

	FILE* f = directory::open("collector.commands","r");
	if(f) {
		char line[1024];
		while(fgets(line,sizeof(line),f))
		{
			line[strlen(line)-1] = 0;
			XtManageChild(XmCreatePushButton(blocks_,line,0,0));
		}
		fclose(f);
	} else {
          /* provide default commands to new users */
	  
          for(int i = 0; i < commands_nb; i++)
            XtManageChild(XmCreatePushButton(blocks_,commands[i],0,0));            
        }

	f = directory::open("collector.history","r");
	if(f)
	{
		char line[1024];
		int n = 0;
		while(fgets(line,sizeof(line),f))
			n++;

		rewind(f);

		XmString* s = new XmString[n];
		int i = 0;

		while(fgets(line,sizeof(line),f))
		{
			line[strlen(line)-1] = 0;
			s[i++] = xec_NewString(line);
		}
		fclose(f);

		XtVaSetValues(command_,
			XmNhistoryItems, s,
			XmNhistoryItemCount, n,
			NULL);
		
		for(i = 0; i < n; i++)
			XmStringFree(s[i]);	
		delete[] s;
	} else {
          /* provide default commands to new users */
	  const int maxCmd = 128;
          XmString* s = new XmString[maxCmd];
          for(int i = 0; i < commands_nb; i++) {
	    s[i] = xec_NewString(commands[i]);
	  }

          XtVaSetValues(command_,
			XmNhistoryItems, s,
			XmNhistoryItemCount, commands_nb,
			NULL);
          for(int i = 0; i < commands_nb; i++) XmStringFree(s[i]);	
          delete[] s;
        }

}

collector::~collector()
{

	FILE * f = directory::open("collector.history","w");
	if(f)
	{
		XmString* s = 0;
		int n = 0;

		XtVaGetValues(command_,
			XmNhistoryItems, &s,
			XmNhistoryItemCount, &n,
			NULL);
		
		for(int i = 0; i < n; i++)
		{
			char *p = xec_GetString(s[i]);
			fprintf(f,"%s\n",p);
			XtFree(p);
		}

		fclose(f);
	}
}

void collector::show(node& n)
{
	instance().nodes_.clear();
	instance().add(&n,true);
	instance().update();
}

void collector::applyCB( Widget, XtPointer data)
{
	XmCommandCallbackStruct* cb = (XmCommandCallbackStruct*)data;
	nodes_.clear();

	char *p = xec_GetString(cb->value);
	send(p);
	XtFree(p);
}

void collector::removeCB( Widget, XtPointer )
{
	XmString    *items = 0;
	int count = 0;

	nodes_.clear();

	XtVaGetValues(list_,
		XmNselectedItems,&items,
		XmNselectedItemCount,&count,
		NULL);

	XmListDeleteItems(list_,items,count);
	update();
}

void collector::noneCB( Widget, XtPointer )
{
	nodes_.clear();
	XmListDeselectAllItems(list_);
	update();
}

void collector::allCB( Widget, XtPointer )
{
	nodes_.clear();
	xec_ListSelectAll(list_);
	update();
}

void collector::selectCB( Widget, XtPointer )
{
	update();
}

void collector::update()
{
	int count = 0;
	int total = 0;

	XtVaGetValues(list_,
		XmNselectedItemCount,&count,
		XmNitemCount,&total,
		NULL);

	XtSetSensitive(remove_,  count != 0);
	XtSetSensitive(command_, count != 0);
	XtSetSensitive(all_,     count != total);
	XtSetSensitive(none_,    count != 0);
}


void collector::send(const char* cmd)
{
	XmString* items = 0;
	int count = 0;

	XtVaGetValues(list_,
		XmNselectedItems,&items,
		XmNselectedItemCount,&count,
		NULL);

	cmd_ = cmd;
	nodes_.clear();

	for(int i = 0; i < count ; i++)
		nodes_.add(items[i]);

	next_ = 0;
	runnable::enable();
	XtSetSensitive(stop_,true);
}
		
		
void collector::run()
{
	if(next_ >= nodes_.count())
	{
		nodes_.clear(); 
		runnable::disable();
		XtSetSensitive(stop_,false);
		return;
	}

	XmListDeselectItem(list_,nodes_[next_]);
	XmListSetBottomItem(list_,nodes_[next_]);
	node *n = find(nodes_[next_++]);
	if(n) n->command(cmd_.c_str());
	update();
}

void collector::closeCB(Widget,XtPointer)
{
  // 201207 ABO warnings?	XmListDeleteAllItems(list_);
	XtUnmanageChild(form_);
}

void collector::entryCB(Widget,XtPointer data)   
{
  XmRowColumnCallbackStruct *cb = (XmRowColumnCallbackStruct *) data;
  xec_ReplaceTextSelection(text_,XtName(cb->widget),False);
}

bool collector::keep(node*)
{
	return true;
}

void collector::stopCB(Widget,XtPointer data)  
{
	nodes_.clear();
}
