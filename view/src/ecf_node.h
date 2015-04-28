#ifndef ecf_node_H
#define ecf_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #65 $ 
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

#include "observable.h"
class host;
class ecf_node;
#include "ecflowview.h"
#include <iostream>
#include <deque>
#include <ChangeMgrSingleton.hpp>
class node;
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#if 0
struct ext_node;
struct ext_list {
   int type_;
   char *name_;
   struct ext_list *next;

   const std::string name() {return name_;}
};

struct ext_tree {int type_;
   char *name;
   struct ext_tree *left, *right;
   int mtype, level;
   struct ext_tree *math;
};

struct ext_trigger {
   int type_;
   char *name;
   struct ext_trigger *next_;
   int status_, nid_, num_;
   struct ext_node *parent;
   void* user_ptr;
   int uint_;
   struct ext_node *kids_;
   int mod_no;
   ext_tree* math;
};

struct ext_node {
   int type_;
   char *name_;
   struct ext_node *next_;
   int status_, nid_, num_;
   struct ext_node *parent;
   void* user_ptr_;
   int uint_;
   struct ext_node *kids_;
   int defs_, svds_, flags;
   ext_node *text, *event, *meter, *label, *time, *date,
   *trigger, *complete, *action, *autocm, *late, *limit,
   *inlimit, *wait;
   int rid, tryno_, alias, count;
   char *passwd;
   ext_node *user, *variable;
   int mod, clk, gain;
   time_t stime, btime;
   ext_node *genvars, *repeat, *log, *restore;
   char* name() const {return name_ ? name_ : getenv("USER");}
   const std::string toString() const {return name();}
   int status() const {return status_;}
   int type() const {return type_;}
   int defComplete() const {return defs_ == STATUS_COMPLETE;}
   const std::string& full_name() {
      static std::string fname;
      if (type_ == NODE_SUPER) {fname = "/";}
      else if (parent) {
         fname = parent->full_name();
         if (type_ == NODE_SUITE) {}
         else if (type_== NODE_FAMILY || type_ == NODE_TASK || type_ == NODE_ALIAS)
         fname += "/";
         else
         fname += ":";
         fname += name_;
      }
      return fname;
   }
};
#endif
const char* ecf_node_name( int ii );

class ecf_node_maker {
   virtual node* make( host&, ecf_node* e ) = 0;
protected:
   static std::map<std::string, ecf_node_maker*>& map();
   static std::vector<ecf_node_maker*>& builders();
public:
   static node* make_xnode( host& h, ecf_node* n, std::string type );
};

template<typename T, class W>
class ecf_node_builder : public ecf_node_maker {
   virtual node* make( host& h, ecf_node* e )
   {
      return new W(h, e);
   }
public:
   ecf_node_builder( int type )
   {
      map()[typeid(T).name()] = this;
      builders()[type] = this;
   }
   ~ecf_node_builder()
   {
   }
};

class ExpressionWrapper {
   Node* node_;
   char kind_;
   std::string mem;
public:
   ExpressionWrapper( Node* n, char c );

   const std::string & name() const;
   const std::string & full_name() const;
   const std::string & toString() const;

   std::string expression() const
   {
      if (kind_ == 'c') return node_->completeExpression();
      return node_->triggerExpression();
   }

   AstTop* get_ast_top()
   {
      if (kind_ == 'c') return node_->completeAst();
      return node_->triggerAst();
   }
};

class ecf_node : public boost::enable_shared_from_this<ecf_node>
 , public observable 
{
private:
   ecf_node* parent_;

protected:
   mutable std::string full_name_;
   std::vector<ecf_node*> kids_;
   node *node_;
   char kind_; // 'c' for complete trigger

   const std::string name_;

   ExpressionWrapper *trigger_;
   ExpressionWrapper *complete_;
   friend class ecf_node_maker;

   void counter();

public:
   ecf_node( ecf_node* parent, const std::string& name, char k );

   ~ecf_node();

   static const std::string& no_owner();
   static const std::string& none();
   static const std::string& slash();

   virtual void set_graphic_ptr( node* )
   {
   }
   virtual node* create_node( host& ) = 0;
   virtual node* create_tree( host& h, node* xnode = 0x0 );
   void adopt( node* n )
   {
      node_ = n;
   }
   void nokids( bool own = false );

   void add_kid( ecf_node* k );
#ifdef BRIDGE
   void* user_ptr;
#endif

   virtual int type() const = 0;
   virtual int flags() const = 0;
   virtual const std::string type_name() const;
   virtual void update( const Node*, const std::vector<ecf::Aspect::Type>& );
   virtual void update( const Defs*, const std::vector<ecf::Aspect::Type>& );
   virtual void update_delete( const Node* );
   virtual void update_delete( const Defs* );

   virtual void make_subtree() = 0;

   virtual const std::string toString() const
   {
      return none();
   }
   // virtual const std::string substitute(const std::string& cmd) const { return cmd; }
   virtual int status() const
   {
      return STATUS_UNKNOWN;
   }
   virtual boost::posix_time::ptime status_time() const
   {
      return boost::posix_time::ptime();
   }
   virtual int defstatus() const
   {
      return STATUS_QUEUED;
   }
   virtual int tryno()
   {
      return 0;
   }
   virtual const std::string& name() const
   {
      return name_;
   }
   virtual const std::string& full_name() const
   {
      return full_name_;
   }

   virtual bool is_late()
   {
      return false;
   }
   virtual bool hasZombieAttr()
   {
      return false;
   }
   virtual bool hasTime()
   {
      return false;
   }
   virtual bool hasDate()
   {
      return false;
   }
   virtual bool hasTrigger()
   {
      return false;
   }

   virtual std::ostream& print( std::ostream& s ) const
   {
      return s << none();
   }
   virtual void print( std::ostream& s )
   {
      s << none();
   }
   virtual const std::string& variable( const std::string& name ) const = 0;
   virtual void why( std::ostream & ) const
   {
   }

   virtual std::string get_var( const std::string& name, bool is_gen = false,
                                bool substitute = false )
   {
      return none();
   }

   virtual Limit* get_limit( const std::string& name ) = 0;
   virtual Node* get_node() const = 0;
   virtual const Label& get_label( const std::string& name ) = 0;
   virtual const Event& get_event( const std::string& name ) = 0;
   virtual const Meter& get_meter( const std::string& name ) = 0;
   virtual const Repeat& get_repeat() = 0;

   static const Repeat& crd();

private:
   ecf_node( const ecf_node& );
   ecf_node& operator=( const ecf_node& );

public:
   ecf_node* parent() const
   {
      return parent_;
   }
   node* xnode() const
   {
      return node_;
   }
   char kind()
   {
      return kind_;
   }
   static ecf_node* dummy_node();

   virtual void unlink( bool detach = true );
   virtual void check() const
   {
      if (parent() == 0x0) std::cerr << "# ecf: no parent: " << name() << "\n";
      if (xnode() == 0x0) std::cerr << "# ecf: no xnode:  " << name() << "\n";
   }

   void delvars();
};

template<typename T>
class ecf_concrete_node : public ecf_node, public AbstractObserver {
private:
   T* owner_;
private:
   ecf_concrete_node( const ecf_concrete_node& );
   ecf_concrete_node& operator=( const ecf_concrete_node& );

protected:
   virtual node* create_node( host& h )
   {
      return ecf_node_maker::make_xnode(h, this, type_name());
   }

public:
   ecf_concrete_node( T* owner, ecf_node* parent, char c = 'd' )
     : ecf_node(parent, owner ? owner->name() : ecf_node::none(), c)
     , owner_(owner)
   {
      if (0 && parent) {
         full_name_ = parent->full_name();
         full_name_ += ":";
         full_name_ += name();
      }
   }

   ~ecf_concrete_node()
   {
      unlink();
   }

   virtual void set_graphic_ptr( node* )
   {
   }

   T* get() const
   {
      return owner_;
   }
   virtual const std::string toString() const;

   // virtual const std::string substitute(const std::string& cmd) const { return cmd; }
   virtual int status() const
   {
      return STATUS_UNKNOWN;
   }
   virtual int defstatus() const
   {
      return STATUS_QUEUED;
   }
   virtual boost::posix_time::ptime status_time() const
   {
      return boost::posix_time::ptime();
   }
   virtual int tryno()
   {
      return 0;
   }
   virtual int type() const
   {
      return NODE_UNKNOWN;
   }
   virtual int flags() const
   {
      return 0;
   }
   virtual const std::string& name() const
   {
      return ecf_node::name();
   }
   virtual const std::string& full_name() const
   {
      return full_name_;
   }
   virtual bool is_late()
   {
      return false;
   }
   virtual bool hasZombieAttr()
   {
      return false;
   }
   virtual bool hasTime()
   {
      return false;
   }
   virtual bool hasDate()
   {
      return false;
   }
   virtual bool hasTrigger()
   {
      return false;
   }

   virtual void make_subtree()
   {
   }

   virtual std::ostream& print( std::ostream& s ) const
   {
      return s << none();
   }
   virtual void print( std::ostream& s )
   {
   }

   virtual const std::string type_name() const
   {
      return typeid(owner_).name();
   }

   virtual void update( const Node* n, const std::vector<ecf::Aspect::Type>& asp )
   {
      if (!owner_) return;
      ecf_node::update(n, asp);
   }
   virtual void update( const Defs* n, const std::vector<ecf::Aspect::Type>& asp )
   {
      if (!owner_) return;
      ecf_node::update(n, asp);
   }

   void update_delete( const Node* n )
   {
      unlink();
      ecf_node::update_delete(n);
   }
   void update_delete( const Defs* n )
   {
      unlink();
      ecf_node::update_delete(n);
   }

   virtual const std::string& variable( const std::string& ) const
   {
      return none();
   }
   virtual void why( std::ostream & ) const
   {
   }

   virtual std::string get_var( const std::string& name, bool is_gen = false,
                                bool substitute = false )
   {
      return none();
   }

   virtual void unlink( bool detach = true )
   {
      owner_ = 0x0;
      ecf_node::unlink(detach);
   }
   virtual void check() const
   {
      if (get() == 0x0) std::cerr << "# ecf: no owner: " << name() << "\n";
      ecf_node::check();
   }

   virtual Limit* get_limit( const std::string& name )
   {
      return 0x0;
   }
   virtual Node* get_node() const
   {
      return 0x0;
   }
   virtual const Label& get_label( const std::string& name )
   {
      return Label::EMPTY();
   }
   virtual const Event& get_event( const std::string& name )
   {
      return Event::EMPTY();
   }
   virtual const Meter& get_meter( const std::string& name )
   {
      return Meter::EMPTY();
   }
   virtual const Repeat& get_repeat()
   {
      return crd();
   }

private:
   bool is_reset( const std::vector<ecf::Aspect::Type>& v ) const
   {
      bool reset = false;
      for(std::vector<ecf::Aspect::Type>::const_iterator it = v.begin(); it != v.end();
               ++it) {
         if (*it == ecf::Aspect::ORDER || *it == ecf::Aspect::ADD_REMOVE_NODE
                  || *it == ecf::Aspect::ADD_REMOVE_ATTR) {
            reset = true;
            break;
         }
      }
      return reset;
   }
};

template<typename T>
ecf_node* make_node( T* n, ecf_node* parent, char c = 'd' )
{
   ecf_node* ec = new ecf_concrete_node<T>(n, parent, c);
   if (ec && n) {
      int type = ec->type();
      // gcc 4.7 optimisation issue, keep next line
      // XECFDEBUG { if (!ec) std::cerr << "# make node " << type << "\n"; }
      if (!parent || type == NODE_SUPER || type == NODE_SUITE)
         ec->make_subtree();
      else if (type == NODE_FAMILY || type == NODE_TASK || type == NODE_ALIAS) {
         /* temp on demand:: */ec->make_subtree();
      }
   }
   // XECFDEBUG { if (!ec) std::cerr << "# no ecf\n"; if (!n) std::cerr << "# no node\n"; }
   return ec;
}

template<typename T>
ecf_node* make_node( T& n, ecf_node* parent, const char c = 'd' )
{
   return make_node<T>(&n, parent, c);
}

template<typename T>
node* make_xnode( T* n, ecf_node* parent, host& h, char c = 'd' )
{
   ecf_node* ec = make_node<T>(n, parent, c);
   if (ec) {
      node *xnode = ec->create_tree(h);
      ec->adopt(xnode); /* twice ? create is adoption */
      return xnode;
   }
   XECFDEBUG { if (!ec) std::cerr << "# no ecf2\n"; }
   return NULL;
}

template<typename T>
node* make_xnode( T& n, ecf_node* parent, host& h, const char c = 'd' )
{
   return make_xnode<T>(&n, parent, h, c);
}

template<typename T>
void make_kids_list( ecf_node* parent, const std::vector<boost::shared_ptr<T> >& v )
{
   for(typename std::vector<boost::shared_ptr<T> >::const_reverse_iterator j = v.rbegin();
            j != v.rend(); ++j) {
      parent->add_kid(make_node((*j).get(), parent));
   }
}

template<typename T>
void make_kids_list( ecf_node* parent, const std::vector<T>& v )
{
   for(typename std::vector<T>::const_reverse_iterator j = v.rbegin(); j != v.rend();
            ++j) {
      parent->add_kid(make_node(*j, parent));
   }
}

template<typename T>
const std::string ecf_concrete_node<T>::toString() const
{
   if (owner_) 
     return owner_->toString();
   return "";
}

#endif
