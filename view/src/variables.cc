//=============================================================================================
// Name        :
// Author      :
// Revision    : $Revision: #19 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//=============================================================================================

#include "variables.h"
#include "variable_node.h"
#include "node.h"
#include "host.h"
#include "ecf_node.h"

#include <Xm/List.h>
#include <Xm/Text.h>
#include "confirm.h"

extern "C" {
#include "xec.h"
}

variables::variables( panel_window& w )
         : panel(w), loading_(false)
{
}

variables::~variables()
{
   clear();
}

void variables::clear()
{
   loading_ = true;
   XmListDeleteAllItems(list_);
   XtSetSensitive(edit_, False);
   XmTextSetString(name_, (char*) "");
   XmTextSetString(value_, (char*) "");
   loading_ = false;
}

struct cless_than {
   inline bool operator()( const Variable& v1, const Variable& v2 )
   {
      return (v1.name() < v2.name());
   }
};

void variables::show( node& n )
{
   loading_ = true;

   int varsize = 0;
   int valsize = 0;
   char fmt1[256];
   char fmt2[256];
   char fmt3[256];
   node* m = &n;

   XtSetSensitive(edit_, True);
   XmListDeleteAllItems(list_);
   std::vector<Variable> gvar;
   std::vector<Variable>::const_iterator it, gvar_end;
   ecf_node* prox;

   while ( m != 0 ) {
      /* for (node* run = m->kids(); run; run = run->next())
       if (run->type() == NODE_VARIABLE) {
       varsize = std::max(varsize, (int) run->name().size());
       valsize = std::max(valsize, (int) ((variable_node*) run)->get_var().size());
       } */
      {
         prox = m->__node__();
         if (!prox) return;

         Defs* defs = 0;
         Node* ecf = 0;
         if (dynamic_cast<ecf_concrete_node<Node>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Node>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Task>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Task>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Family>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Family>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Suite>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Suite>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Defs>*>(prox)) {
            defs = dynamic_cast<ecf_concrete_node<Defs>*>(prox)->get();
         }
         if (!ecf && !defs) {
            break;
         }

         if (ecf ) {
            gvar.clear();
            ecf->gen_variables(gvar);
            for(it = gvar.begin(); it != gvar.end(); ++it) {
               varsize = std::max(varsize, (int) (*it).name().size());
               valsize = std::max(valsize, (int) (*it).theValue().size());
            }

            gvar = ecf->variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
               varsize = std::max(varsize, (int) (*it).name().size());
               valsize = std::max(valsize, (int) (*it).theValue().size());
            }
         }
         if (defs) {
            const std::vector<Variable>& gvar = defs->server().user_variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
                varsize = std::max(varsize, (int) (*it).name().size());
                valsize = std::max(valsize, (int) (*it).theValue().size());
            }
            const std::vector<Variable>& var = defs->server().server_variables();
            for(it = var.begin(); it != var.end(); ++it) {
                varsize = std::max(varsize, (int) (*it).name().size());
                valsize = std::max(valsize, (int) (*it).theValue().size());
            }
         }
      }
      m = m->parent();
   }

   if (!varsize) return;
   snprintf(fmt1, 256, "(%%-%ds = %%-%ds)", varsize, valsize);
   snprintf(fmt2, 256, " %%-%ds = %%-%ds ", varsize, valsize);
   snprintf(fmt3, 256, "[%%-%ds = %%-%ds]", varsize, valsize);
   {
     std::vector<std::string> shown;     
      char buffer[1024];
      node *m = &n;
      while ( m != 0 ) {
	snprintf(buffer, 1024, "Variables defined for %s %s", m->type_name(), m->name().c_str());
         xec_AddFontListItem(list_, buffer, 1);
         {
            prox = m->__node__();
            if (!prox) break;

            Defs* defs = 0;
            Node* ecf = 0;
            if (dynamic_cast<ecf_concrete_node<Node>*>(prox)) {
               ecf = dynamic_cast<ecf_concrete_node<Node>*>(prox)->get();
            }
            else if (dynamic_cast<ecf_concrete_node<Task>*>(prox)) {
               ecf = dynamic_cast<ecf_concrete_node<Task>*>(prox)->get();
            }
            else if (dynamic_cast<ecf_concrete_node<Family>*>(prox)) {
               ecf = dynamic_cast<ecf_concrete_node<Family>*>(prox)->get();
            }
            else if (dynamic_cast<ecf_concrete_node<Suite>*>(prox)) {
               ecf = dynamic_cast<ecf_concrete_node<Suite>*>(prox)->get();
            }
            else if (dynamic_cast<ecf_concrete_node<Defs>*>(prox)) {
                defs = dynamic_cast<ecf_concrete_node<Defs>*>(prox)->get();
            }
            if (!ecf && !defs) break;


            if (ecf) {
               gvar.clear();
               gvar = ecf->variables();
               std::sort(gvar.begin(), gvar.end(), cless_than());
               gvar_end = gvar.end();
               for(it = gvar.begin(); it != gvar_end; ++it) {
		  const std::string& name = (*it).name();
		  if (std::find(shown.begin(), shown.end(), name) == shown.end()) {
		    snprintf(buffer, 1024, fmt2, name.c_str(), (*it).theValue().c_str());
		    xec_AddFontListItem(list_, buffer, 0);
		    shown.push_back(name); }
               }

               ecf->gen_variables(gvar);
               for(it = gvar.begin(); it != gvar.end(); ++it) {
                  if ((*it).name() == "" || 
		      *it == Variable::EMPTY() || 
		      (*it).name() == "ECF_PASS") 
		    continue;
		  const std::string& name = (*it).name();
		  if (std::find(shown.begin(), shown.end(), name) == shown.end()) {
		    snprintf(buffer, 1024, fmt1, name.c_str(), (*it).theValue().c_str());
		    xec_AddFontListItem(list_, buffer, 0);
		    shown.push_back(name); }
               }
            }

            if (defs) {
               gvar = defs->server().user_variables();
               std::sort(gvar.begin(), gvar.end(), cless_than());
               for(it = gvar.begin(); it !=  gvar.end(); ++it) {
		  const std::string& name = (*it).name();
		  if (std::find(shown.begin(), shown.end(), name) == shown.end()) {
		 snprintf(buffer, 1024, fmt2, name.c_str(), (*it).theValue().c_str());
		 xec_AddFontListItem(list_, buffer, 0);
		    shown.push_back(name); }
               }

               gvar = defs->server().server_variables();
               for(it = gvar.begin(); it !=  gvar.end(); ++it) {
		  const std::string& name = (*it).name();
		  if (std::find(shown.begin(), shown.end(), name) == shown.end()) {
		    bool readOnly = name=="ECF_NODE" || name=="ECF_HOST" || name=="ECF_PORT" ||
		      name=="ECF_LISTS" || // security ???
		      name=="ECF_PID"  || name=="ECF_VERSION";
		    snprintf(buffer, 1024, readOnly ? fmt3 : fmt1, 
			     name.c_str(), (*it).theValue().c_str());
		    xec_AddFontListItem(list_, buffer, 0);
		    shown.push_back(name); }
               }
            }
         }
         m = m->parent();
      }
   }
   loading_ = false;
}

Boolean variables::enabled( node& n )
{
   int type = n.type();
   if (type == NODE_SUPER || type == NODE_FAMILY || type == NODE_TASK 
       || type == NODE_ALIAS) 
     return True;
   for(node* run = n.kids(); run; run = run->next())
      if (run->type() == NODE_VARIABLE) 
	return True;
   return False;
}

void variables::browseCB( Widget w, XtPointer data )
{
   XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
   char *p = xec_GetString(cb->item);
   if (*p == 'V') {
      XmTextSetString(name_, (char*) "");
      XmTextSetString(value_, (char*) "");
   }
   else {
      char *q = p + 1;
      char *r = p + 1;

      while ( *r && *r != '=' )
         r++;

      *r = 0;

      while ( *q && q[strlen(q) - 1] == ' ' )
         q[strlen(q) - 1] = 0;

      r += 2;

      if (*p == '(') r[strlen(r) - 1] = 0;
      if (*p == '[') r[strlen(r) - 1] = 0;

      while ( *r && r[strlen(r) - 1] == ' ' )
         r[strlen(r) - 1] = 0;

      XmTextSetString(name_, q);
      XmTextSetString(value_, r);
   }
   nameCB(w, data);
   valueCB(w, data);

   XtFree(p);
}

/* from: http://www.ist.co.uk/motif/books/vol6A/ch-13.fm.html */
/* find the item in the list that matches the specified pattern */
#include <Xm/TextF.h>
#define _REGEX_RE_COMP
#include <regex.h>
void search_item( Widget text_w, XtPointer client_data, XtPointer call_data, Widget nam,
                  Widget val )
{
   Widget list_w = (Widget) client_data;
   char *exp, *text, *newtext = XmTextFieldGetString(text_w);
   XmString *strlist, *selectlist = NULL;
   int cnt, j = 0;

   if (!newtext || !*newtext) {
      XtFree(newtext);
      return;
   }

   if ((exp = re_comp(newtext))) {
      printf("Error with re_comp(%s): %s\n", newtext, exp);
      XtFree(newtext);
      return;
   }

   XtVaGetValues(list_w, XmNitemCount, &cnt, XmNitems, &strlist, NULL);

   while ( cnt-- ) {
      if (!(text = (char *) xec_GetString(strlist[cnt]))) break;

      if (re_exec(text) > 0) {
         selectlist = (XmString *) XtRealloc((char *) selectlist, (j + 1) * (sizeof(XmString *)));
         selectlist[j++] = XmStringCopy(strlist[cnt]);

         char *p = xec_GetString(strlist[cnt]);
         char *q = p + 1;
         char *r = p + 1;
         while ( *r && *r != '=' )
            r++;
         *r = 0;
         while ( *q && q[strlen(q) - 1] == ' ' )
            q[strlen(q) - 1] = 0;
         r += 2;
         if (*p == '(') r[strlen(r) - 1] = 0;
         if (*p == '[') r[strlen(r) - 1] = 0; // readOnly variables
         while ( *r && r[strlen(r) - 1] == ' ' )
            r[strlen(r) - 1] = 0;
         XmTextSetString(nam, q);
         XmTextSetString(val, r);
         *r = '=';
         XtFree(p);
      }
      XtFree(text);
   }
   free(exp);
   XtFree(newtext);

   XtVaSetValues(list_w, XmNselectedItems, selectlist, XmNselectedItemCount, j, NULL);

   while ( j-- )
      XmStringFree(selectlist[j]);
   // XmTextFieldSetString (text_w, "");
}
/* */

void variables::findCB( Widget, XtPointer )
{
   char *name = XmTextGetString(name_);
   search_item(name_, list_, NULL, name_, value_);
   XtFree(name);
}

void variables::deleteCB( Widget, XtPointer )
{
   if (get_node()) {
      char *name = XmTextGetString(name_);
      const char* fullname = get_node()->full_name().c_str();
      if (confirm::ask(False, "Delete variable %s for node %s", name, fullname)) {
         // repeat get_node while suite may have been cancelled by another
         // while answering this question
         if (get_node()) {
            if (get_node()->__node__()) /* ecflow */
               get_node()->serv().command(clientName, "--alter", "delete", "variable", name,
                                          fullname, NULL);
            else
               get_node()->serv().command("alter", "-vr", fullname, name, NULL);
         }
      }
      XtFree(name);
      update();
   }
   else
      clear();
}

void variables::setCB( Widget, XtPointer )
{
   if (get_node()) {

      char *name = XmTextGetString(name_);
      char *value = XmTextGetString(value_);
      Boolean ok = True;
      node* n = get_node()->variableOwner(name);

      if (n != 0 && n != get_node()) {
         ok = confirm::ask(True, "This variable is already defined in the %s %s\n"
                           "A new variable will be created for the selected node\n"
                           "and hide the previous one\n"
                           "Do you want to proceed?",
                           n->type_name(), n->full_name().c_str());
      }

      if (n != 0 && n->isGenVariable(name) && ok) {
	ok = confirm::ask(True, "This variable is a generated variable\n"
			  "Do you want to proceed?");
      }

      if (ok) {
         bool add = true;
         if (get_node()->__node__()) {
	   add = get_node()->__node__()->variable(name) == ecf_node::none();
	   get_node()->serv().command(clientName, "--alter", add ? "add" : "change", "variable",
				      name, value, get_node()->full_name().c_str(), NULL);
         } else
            get_node()->serv().command("alter", "-v", get_node()->full_name().c_str(), name, value,
                                       NULL);
         if (add) update();
      }
      XtFree(name);
      XtFree(value);
   }
   else
      clear();
}

void variables::nameCB( Widget, XtPointer )
{
   if (loading_) return;

   char *p = XmTextGetString(name_);
   if (get_node()) {
      node *n = get_node()->variableOwner(p);
      XtSetSensitive(delete_, n != 0 && (!n->isGenVariable(p) || n != get_node()));
   }
   else {
      clear();
   }
   XtFree(p);
}

void variables::valueCB( Widget, XtPointer )
{
   if (loading_) return;

   char *p = XmTextGetString(name_);
   char *v = XmTextGetString(value_);
   if (get_node()) {
      const char *x = get_node()->variable(p).c_str();
      XtSetSensitive(set_, (x == 0 || strcmp(x, v) != 0) && v[0] != 0);
   }
   else {
      clear();
   }
   XtFree(v);
   XtFree(p);
}

