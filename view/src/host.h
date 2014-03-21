//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #33 $ 
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

#ifndef host_H
#define host_H

#include <time.h>
#include <stdio.h>

#include "ecf_node.h"
#ifndef extent_H
#include "extent.h"
#endif

#ifndef timeout_H
#include "timeout.h"
#endif

#ifndef tmp_file_H
#include "tmp_file.h"
#endif

#ifndef searchable_H
#include "searchable.h"
#endif

#ifndef observable_H
#include "observable.h"
#endif
#include "text_lister.h"

class node;
class tree;
#ifndef lister_H
#include "lister.h"
#endif

#ifndef option_H
#include "option.h"
#endif

#ifndef configurable_H
#include "configurable.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef SINGLETON_H
#include "singleton.h"
#endif

class Client;
class configurator;
class host;
class url;
#include <map>

class host_maker {
  virtual host* make(const std::string& name, const std::string& machine, int port) = 0;
protected:
  static std::map<int, host_maker*> map_;
  static int port_max;
public:
  static host* make_host(std::string name, std::string machine, int port);
  virtual ~host_maker() {}
};

template<class T> 
class host_builder : public host_maker {
  virtual host* make(const std::string& name, const std::string& machine, int port) 
  { return new T(name, machine, port); }
public:
  host_builder(int port_max) { map_[port_max] = this; }
};

class host_lister {
public:
	virtual void next(host&) = 0;
	virtual ~host_lister() {}
};


class host : public extent<host>
, public timeout
  , public searchable
  , public configurable
  , public observable 
{
 public:
	static void status(Boolean);
	static void login(const std::string&,int);
	static void login(const std::string&);
 	static void logout(const std::string&);
	static void broadcast(bool = false);
	static void check_all_mail();

	static node* find(const std::string&,const std::string&);
	static host* find(const std::string&);
	static void redraw_all();
	static void plug(node*);
	static void comp(node*, const char* a, const char* b );
	static host* new_host(const std::string&,const std::string&,int);
	static void  remove_host(const std::string&);
	static host& dummy();
	static void chat();

 protected:
	static host* find(const std::string&,int);
	void update_reg_suites(bool ) {};

  static void hosts(host_lister&);
  static int maxLines;
  host( const std::string& name, const std::string& host, int number );

 public:
  virtual const std::string reply() const;

  virtual ~host();

  void call_after_commands(bool b) { after_command_ = b; }
  
  static void init();

  virtual void login();
  virtual void logout();
  int status();
  void check_mail();

  virtual int update();
  virtual int command(const std::string& str) { return TRUE; };
  virtual int command(const char*,...) { return TRUE; };
  virtual int command(int,char**) { return TRUE; };
  virtual std::list<std::string>& history(std::string&);
  node* top() { return top_; }
  tree* where() { return tree_; }

  virtual tmp_file file(node& n,std::string name);
  virtual tmp_file sfile( node& n, std::string name );
  virtual tmp_file manual(node&);
  virtual tmp_file script(node&);
  virtual tmp_file output(node&);
  virtual tmp_file job(node&);
  virtual tmp_file jobstatus(node&, const std::string &);
  virtual tmp_file jobcheck(node&, const std::string &);

  virtual tmp_file edit(node& n,std::list<Variable>& l,Boolean preproc);
  virtual void send(node& n,Boolean alias,Boolean run,NameValueVec& v,const char* file);
  virtual void dir(node&,const char*,lister<ecf_dir>&);

  void send(const char*,std::vector< std::string >&);

  virtual bool zombies(int mode,const char* name);
  virtual bool get_zombies_list(std::vector<std::string>& list); 

  const std::vector<std::string>& suites() const;
  virtual void suites(int, std::vector<std::string>&) {};
  void suites(node*,bool=true);

  void redraw(bool create=false);
  virtual bool create_tree(int hh=0, int min=0, int sec=0) { return false; } 
	time_t last() { return last_; }

	virtual void change(const std::string&,const std::string&,int);

	void aborted(node&);
	void restarted(node&);
	void late(node&);
	void zombie(node&);
	void to_check(node&);

	void changed(resource&);

	virtual str logfile() const ;
	static int do_plug(node*,node*);
	static int do_comp(node*,node*, 
                            const std::string& kind, const std::string& meth);

	// From timeout
	void run();

	// From searchable
	inline const char* name() const { return name_.c_str(); }
	const std::string& name_ref() const { return name_; }

	const char* machine() const { return host_.c_str(); }
	int number()  const      { return number_; }
	void search(node_lister&);

	void timefile(const str& s) { timefile_ = s; }
	const str& timefile();
	
	virtual const std::vector<std::string>& messages(const node&n) const;

 protected:
	host(const host&);
	host& operator=(const host&);

	void destroy_top(node*) const;

	str     host_;
	int     number_;

	std::string name_;

 	bool    connected_;
	bool    after_command_;

	str     user_;
	str     passwd_;
	virtual int origin() const;

	option<int>       timeout_;
	option<int>       maximum_;
	option<bool>      drift_;
	option<bool>      connect_;
	option<std::vector<std::string> > suites_;

	option<bool>      aborted_;

	option<bool>      restarted_;
	option<bool>      late_;
	option<bool>      poll_;

	option<bool>      direct_read_;
	option<bool>      new_suites_;

	option<bool>      zombie_;
	option<bool>      aliases_;
	option<bool>      to_check_;

	bool  chkmail_;

	node*       top_;
	tree*       tree_;
	int         mail_;
	time_t      last_;
	str         timefile_;
	std::list<std::string> hist_;
	int history_len_;
	std::string loghost_;
	std::string logport_;

	virtual void reset(bool full=false, bool sync=true) {};
	virtual bool connect_mngt(bool connect);

	bool updating_; // SUP-423
 public:
	void updating(bool b) { updating_ = b; }
        virtual void stats(std::ostream& f) { };
};

class ehost : public host {
 public:
  ehost( const std::string& name,const std::string& h,int number );
  virtual ~ehost();
  virtual void dir(node&,const char*,lister<ecf_dir>&);

  bool zombies(int mode, const char *name);
  bool get_zombies_list(std::vector<std::string>& list); 

  void login();
  void logout();
  void changed( resource& r );
  virtual std::list<std::string>& history(std::string&);

  virtual str logfile() const ;
  virtual int command(int argc, char **argv);
  int command(const std::string& str);
  int command(const char* cmd, ... );

  virtual tmp_file edit(node& n,std::list<Variable>& l,Boolean preproc);
  virtual void send(node& n,Boolean alias,Boolean run,NameValueVec& v,const char* file);
  virtual void suites(int, std::vector<std::string>&) ;
  tmp_file sfile(node&,std::string);
  tmp_file file(node& n,std::string name);
  tmp_file manual( node& n );
  tmp_file jobcheck( node& n, const std::string &cmd );
  tmp_file jobstatus( node& n, const std::string &cmd );
  
  const std::string reply() const;
  virtual int update();
  
  virtual const std::vector<std::string>& messages(const node&n) const;

  void stats(std::ostream& f);
 protected:
  virtual bool connect_mngt(bool connect);
  ClientInvoker client_;
  bool create_tree(int hh=0, int min=0, int sec=0);

  virtual void reset(bool full=false, bool sync=true);
 protected:
	void update_reg_suites(bool get_ch_suites);

	ehost(const ehost&);
	ehost& operator=(const ehost&);
};
struct host_locker {
protected:
   host* host_;
   int e_;
public:
   host_locker( host* h );
   ~host_locker();
   int err() { return e_; }
};

class Updating {
public:
 Updating(host* h) : host_(h) {
    do_full_redraw_ = false;
    host_->updating(true);
  }

  ~Updating() {
    host_->updating(false);
  }

  static void set_full_redraw() {
    do_full_redraw_ = true;
  }

  static bool full_redraw() {
    return do_full_redraw_;
  }

private:
  host* host_;
  static bool do_full_redraw_;
};

#endif

