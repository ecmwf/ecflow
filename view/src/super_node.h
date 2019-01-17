#ifndef super_node_H
#define super_node_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #12 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/


#ifndef simple_node_H
#include "simple_node.h"
#endif
#ifndef host_H
#include "host.h"
#endif
#ifndef timeout_H
#include "timeout.h"
#endif
class super_node : public simple_node, public timeout {
  virtual void why(std::ostream&);
	virtual Boolean visible() const { return True; }
	virtual Boolean show_it() const { return True; } 
	virtual const std::string& name() const { return serv().name_ref(); } 
	virtual const std::string& node_name() const { return serv().name_ref(); } 
	virtual const char* type_name() const { return "server"; } 
	virtual void run();
	virtual void up_to_date();
	virtual void active(bool);
	virtual void drawBackground(Widget w,XRectangle* r,bool);
	virtual void info(std::ostream&);

	virtual bool trigger_kids() const { return false; }
	virtual bool trigger_parent() const { return false; }
	virtual bool timeTable() { return true; }

	Boolean isLocked() const { return ecfFlag(FLAG_LOCKED); }

	virtual Boolean hasManual() const   { return False; }
	virtual Boolean hasTriggers() const { return False; }
	virtual Boolean hasInfo() const     { return True; }

	bool active_;
	int decay_;

public:
 super_node(host& h,ecf_node* n)
   : simple_node(h,n) 
	  , timeout(60)
	  , active_(true)
	  , decay_(0)
	  { folded_ = False;  enable(); }

#ifdef BRIDGE
 super_node(host& h,sms_node* n, char b)
   : simple_node(h,n,b) 
	  , timeout(60)
	  , active_(true)
	  , decay_(0)
	  { folded_ = False;  enable(); }
#endif
};

#endif
