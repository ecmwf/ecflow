#ifndef simple_node_H
#define simple_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #18 $ 
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


#ifndef node_H
#include "node.h"
#endif

class simple_node : public node {
public:
  simple_node(host&,ecf_node*);
#ifdef BRIDGE
  simple_node(host&,sms_node*,char);
#endif
  ~simple_node();
  
  virtual std::string variable(const std::string&, bool substitute=false);
  virtual const char* status_name() const;

  virtual void info(std::ostream&);
  virtual void triggers(trigger_lister&);
  
  virtual Boolean hasTriggers() const;
  virtual Boolean hasDate() const;
  virtual Boolean hasTime() const;
  virtual Boolean hasManual() const;
  virtual Boolean isSimpleNode() const { return True; }
  virtual Boolean isGenVariable(const char*);
  virtual void genvars(std::vector<Variable>&);
  virtual void variables(std::vector<Variable>&);
  
  virtual Boolean visible() const;
  virtual Boolean visible_kid() const;
  virtual Boolean show_it() const;
  
  virtual void tell_me_why(std::ostream&);
  virtual void why(std::ostream&);
  virtual void suspended(std::ostream&);
  virtual void aborted(std::ostream&);
  virtual void queued(std::ostream&);
  
  virtual int tryno()  const;
  virtual int flags() const;
  virtual boost::posix_time::ptime status_time() const;
  
  virtual Boolean isMigrated() const  { return ecfFlag(FLAG_MIGRATED); }
  virtual Boolean isLate() const      { return ecfFlag(FLAG_LATE);     }
  virtual Boolean isWaiting() const   { return ecfFlag(FLAG_WAIT);     }
  virtual Boolean hasMessages() const;
  
  virtual Boolean isTimeDependent() const { return hasDate() || hasTime(); }
  
  virtual Boolean isForceAbort() const    { return  ecfFlag(FLAG_FORCE_ABORT); }
  virtual Boolean isUserEdit() const    { return  ecfFlag(FLAG_USER_EDIT); }
  virtual Boolean isTaskAbort() const    { return  ecfFlag(FLAG_TASK_ABORTED); }
  virtual Boolean isEditFailed() const    { return  ecfFlag(FLAG_EDIT_FAILED); }
  virtual Boolean isCmdFailed() const    { return  ecfFlag(FLAG_CMD_FAILED); }
  virtual Boolean isScriptMissing() const    { return  ecfFlag(FLAG_NO_SCRIPT); }
  virtual Boolean isKilled() const    { return  ecfFlag(FLAG_KILLED); }
  virtual Boolean isByRule() const    { return  ecfFlag(FLAG_BYRULE); }
  virtual Boolean isQueueLimit() const    { return  ecfFlag(FLAG_QUEUELIMIT); }
  
  virtual Boolean isRerun() const     { return  tryno() > 1; }
  virtual Boolean isZombie() const;
  virtual Boolean hasZombieAttr() const;
  virtual Boolean isToBeChecked() const;
  virtual Boolean isDefComplete() const;
  
  virtual bool trigger_kids() const { return true; }
  virtual bool trigger_parent() const { return true; }
  
  virtual Boolean ecfFlag(int) const;
  
  // void unlink();
  
 protected:
  
  virtual void perlify(FILE*);
  int old_status_, old_tryno_, old_flags_;
 private:
  
  simple_node(const simple_node&);
  simple_node& operator=(const simple_node&);
  
  virtual void drawNode(Widget,XRectangle*,bool);
  virtual void sizeNode(Widget,XRectangle*,bool);
  virtual void drawBackground(Widget,XRectangle*,bool);
  
  void scan(Ast*,trigger_lister&,node*);
  void scan(Ast*,std::ostream&,bool);
  void scan(node*,std::ostream&);
#ifdef BRIDGE
  void scan(sms_tree* m,trigger_lister& f,bool b);
  void scan(sms_tree* m,std::ostream& f,bool b);
  // void scan(sms_tree* m,std::ostream& f,bool b);
#endif
  void scan_limit(Ast*,std::ostream&);
  
  virtual Pixel color() const;
};

inline void destroy(simple_node**) {}

class suite_node : public simple_node {
 public:
  suite_node(host& h,ecf_node* n) : simple_node(h,n) {}
#ifdef BRIDGE
  suite_node(host& h,sms_node* n,char b) : simple_node(h,n,b) {}
#endif
  virtual Boolean show_it() const;
  virtual Boolean visible() const;

  // virtual void info(std::ostream&);
};

class family_node : public simple_node {
  virtual bool trigger_kids()   const { return false; }
  virtual bool trigger_parent() const { return false; }
public:
  family_node(host& h,ecf_node* n) : simple_node(h,n) {}
#ifdef BRIDGE
  family_node(host& h,sms_node* n,char b) : simple_node(h,n,b) {}
#endif
};

#endif
