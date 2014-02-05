#ifndef node_H
#define node_H
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


#include "ecflowview.h"

class host;
class node;
class trigger_lister;
class node_lister;
class node_editor;
class node_data;
class ostream;
class url;
#include "ecf_node.h" // class ecf_node;

#ifndef xnode_H
#include "xnode.h"
#endif

#ifndef observable_H
#include "observable.h"
#endif

#ifndef gui_H
#include "gui.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifndef xmstring_H
#include "xmstring.h"
#endif

#ifdef BRIDGE
extern "C" {
#define new _new
#define delete _delete
#undef NODE_MAX
#include "sms.h"
#include "smsproto.h"
#undef new
#undef delete
}
#endif

template<class T>
class lister;

class node_info {
public:
	virtual ~node_info()      {}
	virtual const str& name() = 0;
};

class node : public observable, public xnode {
  friend class ecf_node;
  friend class tree;
  int type_;
public:

  static bool is_json;

	void scan(node* n) { scan(n,n); }

	virtual void update(int,int,int);
	void remove();

	node* next()  const { return next_; }
	node* kids()  const { return kids_; }
	host& serv() const { return host_; }

	virtual node* parent() const;

	virtual Boolean visible() const;
	virtual Boolean show_it() const;
	virtual Boolean visible_kid() const { return false; }

	virtual Boolean menus()      { return True; }
	virtual Boolean selectable() { return True; }

	virtual const std::string& name() const;
	virtual const std::string& full_name() const;
	virtual const std::string& node_name() const { return name(); }
	virtual const std::string& net_name() const;
	virtual const std::string& parent_name() const; 
	virtual const std::string& definition() const { return full_name(); }

	virtual void adopt(node*);
	virtual void create();

#ifdef BRIDGE
	const std::string name_;
	const std::string full_name_;
	static node* create(host&h,sms_node* n,char = 0);
	static void schanged(sms_node*,int,int,int,void*);
	static node* find(sms_node*);

 protected:
	node(host& h,sms_node* owner, char b);
	sms_node
#else
 protected:
	void
#endif
	  *tree_;
 public:
	virtual void reset();
	void delvars();

	virtual void search(node_lister&);
	virtual void info(std::ostream&);
	virtual const std::string toString() const;
	virtual std::string substitute(const char*);
	virtual void command(const char*);

	virtual void tell_me_why(std::ostream&);
	virtual void why(std::ostream&);
	virtual bool evaluate() const;
	virtual void suspended(std::ostream&);
	virtual void aborted(std::ostream&);
	virtual void queued(std::ostream&);

	virtual void triggers(trigger_lister&);
	virtual void triggered(trigger_lister&);

	Boolean folded()       { return folded_; }
	virtual void folded(Boolean f);

	virtual Boolean ecfFlag(int)  const { return False; }

	virtual void genvars(std::vector<Variable>&) {};
	virtual void variables(std::vector<Variable>&) {};

	virtual const char*         type_name() const;
	virtual const char*         status_name() const;

	void insert(node*);

	virtual int type()   const;
	virtual int status() const;
	virtual boost::posix_time::ptime status_time() const 
	  { return boost::posix_time::ptime(); }

	virtual int tryno()  const { return 0; }
	virtual int flags()  const { return 0; }
	const std::vector<std::string>& messages() const;

	virtual Boolean isSimpleNode()const { return False; }
	virtual Boolean hasTriggers() const { return False; }
	virtual Boolean hasDate() const     { return False; }
	virtual Boolean hasTime() const     { return False; }
	virtual Boolean hasManual() const   { return False; }
	virtual Boolean hasInfo() const     { return True;  }
	virtual Boolean isMigrated() const  { return False; }
	virtual Boolean isLate() const      { return False; }
	virtual Boolean isWaiting() const   { return False; }
	virtual Boolean hasMessages() const { return False; }
	virtual Boolean isTimeDependent() const   { return False; }
	virtual Boolean isRerun() const     { return  False; }
	virtual Boolean isLocked() const    { return  False; }

	virtual Boolean isDefComplete() const     { return False; }
	virtual Boolean isZombie() const    { return  False; }
	virtual Boolean hasZombieAttr() const    { return  False; }
	virtual Boolean isToBeChecked() const    { return  False; }
	virtual Boolean hasText() const    { return  False; }

	virtual Boolean isForceAbort() const    { return  False; }
	virtual Boolean isUserEdit() const    { return  False; }
	virtual Boolean isTaskAbort() const    { return  False; }
	virtual Boolean isEditFailed() const    { return  False; }
	virtual Boolean isCmdFailed() const    { return  False; }
	virtual Boolean isScriptMissing() const    { return  False; }
	virtual Boolean isKilled() const    { return  False; }
	virtual Boolean isByRule() const    { return  False; }
	virtual Boolean isQueueLimit() const    { return  False; }

	Boolean isFolded() const      { return folded_; }

	virtual void active(bool) {}
	virtual void up_to_date() {}

	virtual bool trigger_kids() const { return false; }
	virtual bool trigger_parent() const { return false; }
	virtual bool show_in_dependancies() const { return false; }
	virtual void add_triggered(node*,node*);
	void unlink(bool detach=true);

	void check();
// ---------------------------

	time_t suite_time();
	node* find_trigger(const std::string& name) const;
	node* find_limit(const std::string& path, const std::string& name);
	node* find(const std::string n);
	static const char* find_name(const char* name);
	node* find_match(const char* name);

// ---------------------------

	virtual Pixel color() const;

	virtual void drawNode(Widget,XRectangle*,bool);
	virtual void sizeNode(Widget,XRectangle*,bool);
	virtual void drawBackground(Widget,XRectangle*,bool);

// ---------------

	virtual void edit(node_editor&);
	virtual void apply(node_editor&);

// ---------------

	virtual std::string variable(const std::string&, bool substitute=false);
	virtual node* variableOwner(const char*);
	virtual Boolean isGenVariable(const char*);

	virtual bool is_my_parent(node*) const;

	virtual bool match(const char*);

	virtual node* graph_node() { return this; }

// ----------------

	virtual bool show_in_html(url&) { return false; }

	virtual const char* html_page(url&);

	virtual void html_name(FILE*,url&);
	virtual void html_title(FILE*,url&);

	virtual void html_why(FILE*,url&)       {}
	virtual void html_output(FILE*,url&)    {}
	virtual void html_manual(FILE*,url&)    {}
	virtual void html_script(FILE*,url&)    {}
	virtual void html_job(FILE*,url&)       {}
	virtual void html_jobstatus(FILE*,url&) {}
	virtual void html_kids(FILE*,url&)      {}
	virtual void html_variables(FILE*,url&) {}

	// ------------------------------
	virtual void as_perl(FILE*,bool);
	virtual void perlify(FILE*) = 0;
	virtual const char* perl_class() { return type_name(); }

	void perl_member(FILE*,const char*,const char*);
	void perl_member(FILE*,const std::string&,const std::string&);
	void perl_member(FILE*,const char*,int);
	void perl_member(FILE*,const char*,ecf_list*);

	static void destroy(node*);
	static void changed(ecf_node*,int a=-1,int b=-1,int c=-1,void *d=0x0);
	static node* find(ecf_node*);

	static GC         blackGC()      { return gui::blackGC();   }
	static GC         blueGC()       { return gui::blueGC();    }
	static GC         redGC()        { return gui::redGC();     }
	static XmFontList smallfont()    { return gui::smallfont(); }
	static XmFontList fontlist()     { return gui::fontlist();  }
	static Pixel      colors(int n)  { return gui::colors(n);   }
	static GC         colorGC(int n) { return gui::colorGC(n);  }

	void  helper(void* h) { helper_ = h;    }
	void* helper()        { return helper_; }

	node_info* get_node_info(const str&);
	void add_node_info(node_info*);
	void remove_node_info(const str&);
	void remove_node_info(node_info*);

	ecf_node* __node__() const { if (tree_) return 0x0; return owner_.get(); }
	bool ondemand(bool full=false); 
protected:
	node(host&,ecf_node*);

	virtual ~node(); // Change to virtual if base class

	node*     next_;
	node*     kids_;
	boost::shared_ptr<ecf_node> owner_;
	host&     host_;
	Boolean   folded_;

	void append(node*);
	const xmstring& labelTree();
	const xmstring& labelTrigger();
	static void shadow(Widget,XRectangle*,bool = true);

 protected:
	xmstring   labelTree_;

private:

	node(const node&);
	node& operator=(const node&);

	void*      helper_;
	node_data* data_;
	bool       triggered_;

	node_data* get_node_data();

	void scan(node*,node*);
	void gather_triggered(node*);

	virtual xmstring make_label_tree();
	virtual xmstring make_label_trigger();

	virtual void draw(Widget w,XRectangle* r) { drawNode(w,r,true); }
	virtual void size(Widget w,XRectangle* r) { sizeNode(w,r,true); }
};

#ifdef BRIDGE
class node_builder {
protected:
	static node_builder* builders_[NODE_MAX];
public:
	virtual node* make(host&,sms_node* n,char) = 0;
	static  node* build(host& h,sms_node* n,char b);
};

template<class T> class node_maker : public node_builder {
public:
  node_maker(int n) { builders_[n] = this; }
  virtual node* make(host& h,sms_node* n,char b) { return new T(h,n,b); }
};
#endif
#endif

