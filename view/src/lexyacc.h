/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#include <Xm/Xm.h>
#include "SimpleTime.h"

typedef struct _flags   flags;
typedef struct _menu    menu;
typedef struct _item    item;
typedef struct _action  action;
typedef struct _node    node;

#ifdef hpux
/* hpux X11 headers are wromg */
#undef bcopy
#undef bzero
#undef bcmp
#endif

int   menus_version(int rel, int maj, int min);
menu* menus_chain_menus(menu *a,menu *b);
item* menus_chain_items(item *a,item *b);
void  menus_root(menu *m);
menu* menus_create_2(char* a,item* b);
item* menus_create_6(flags* a,flags* b,char* c,action* d,char* e,int f);

action* menus_command(char* a);
action* menus_window(char* a);
action* menus_separator();
action* menus_sub_menu();

flags* new_flagNone();
flags* new_typeFlag(int a);
flags* new_flagNot(flags* a);
flags* new_flagOr(flags* a,flags* b);
flags* new_flagAnd(flags* a,flags* b);

flags* new_flagAll();
flags* new_eventFlag(int a);
flags* new_selectionFlag();
flags* new_statusFlag(int a);
flags* new_userFlag(int a);

flags* new_procFlag_node_hasTriggers();
flags* new_procFlag_node_hasDate();
flags* new_procFlag_node_hasTime();
flags* new_procFlag_node_hasText();
flags* new_procFlag_node_isMigrated();
flags* new_procFlag_node_isLocked();
flags* new_procFlag_node_isZombie();

action* menus_internal_host_plug();
action* menus_internal_host_compare(char*a, char*b);

void log_event_meter_event(DateTime* a,node* b,int c);
void log_event_event_event(DateTime* a,node* b,int c);
void log_event_status_event(DateTime* a,node* b,int c);
node *log_event_find(char* a);
