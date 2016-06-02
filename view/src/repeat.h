#ifndef REPEAT_H
#define REPEAT_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #23 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#include "repeat_node.h" 

/**********************************************/
class repeat_date_node : public repeat_node {
  virtual int last() const {
    return (ecf_repeat_date_to_julian(repeat_node::last()) -
            ecf_repeat_date_to_julian(repeat_node::start())) / step() + ink;
  }

  virtual int current() const {
     if (owner_ && get()) {
        int cur = (ecf_repeat_date_to_julian(get()->index_or_value()) -
              ecf_repeat_date_to_julian(start())) / step();
        int gui = ecf_repeat_julian_to_date(ecf_repeat_date_to_julian(start()) + cur * step());

        if (get()->index_or_value() != gui) {
           std::stringstream ss;
           ss << "# WAR repeat value does not match: "
                 << get()->index_or_value()
                 << ", gui "   << gui
                 << ", index " << cur
                 << ", start " << start()
                 << ", last "   << last()
                 << ", step "  << step() << "\n";

           std::string msg (ss.str());
           serv().command(clientName, "--msg", msg.c_str(), NULL);
           std::cerr << msg;
        }
        return cur;
     }
     return repeat_node::current(); }

  virtual void value(char* n,int i) const {
    if (n) {
       sprintf(n,"%ld",
               (ecf_repeat_julian_to_date 
                (ecf_repeat_date_to_julian(start()) + i * step())));
    }
 } 
  virtual const char* perl_class() { return "repeat::date"; }

public:
 repeat_date_node(host& h,ecf_node* r) : repeat_node(h,r) {}
};

/**********************************************/
class repeat_integer_node : public repeat_node {
  virtual int last() const {
    return (repeat_node::last() - repeat_node::start()) / step() + ink; }
  virtual int current() const {
    return (repeat_node::current() - repeat_node::start()) / step(); }
  virtual void value(char*n,int i) const {
    if (n) sprintf(n,"%d",start() + i*step()); }
  virtual const char* perl_class() { 
    return "repeat::integer"; }

public:
 repeat_integer_node(host& h,ecf_node* r) : repeat_node(h,r) {}
};

/**********************************************/
class repeat_day_node : public repeat_node {
  virtual int last() const {
    return (repeat_node::last() - repeat_node::start()) / step() + ink; }
  virtual int current() const { 
    return step(); }
  virtual void value(char*n,int i) const {
    if (n) sprintf(n,"%d",step()); }
  virtual const char* perl_class() { 
    return "repeat::day"; }

public:
 repeat_day_node(host& h,ecf_node* r) : repeat_node(h,r) {}
};

/**********************************************/
class repeat_string_node : public repeat_node {
  virtual int last() const { 
    if (owner_) 
      return repeat_node::last() + ink;
    return repeat_node::last(); }
  virtual int current() const { 
    return repeat_node::current(); }
  virtual void value(char* n,int i) const {
    if (n && get()) 
      sprintf(n,"%s",get()->value_as_string(i).c_str()); }
  virtual int index(const char*) const { 
    return current(); }
  virtual bool can_use_text() { 
    return false; }
  virtual void perlify(FILE* f) {
    if (get()) 
      perl_member(f, "values",get()->toString().c_str()); 
    repeat_node::perlify(f); }
  virtual const char* perl_class() { 
    return "repeat::string"; }

public:
  repeat_string_node(host& h,ecf_node* r) : repeat_node(h,r) {}
};

/**********************************************/
class repeat_enumerated_node : public repeat_node {
  virtual int last() const { 
    if (owner_) 
      return repeat_node::last() + ink;
    return repeat_node::last(); }
  virtual int current() const { 
    return repeat_node::current(); }
  virtual void value(char* n,int i) const {
    if (n && get()) sprintf(n,"%s",get()->value_as_string(i).c_str()); }
  virtual int index(const char*) const {
    return current(); }
  virtual bool can_use_text() { 
    return false; }
  
  virtual void perlify(FILE* f) {
    if (get()) 
      perl_member(f, "values",get()->toString().c_str()); 
    repeat_node::perlify(f); }

  virtual const char* perl_class() { 
    return "repeat::enumerated"; }

public:
  repeat_enumerated_node(host& h,ecf_node *r) : repeat_node(h,r) {}
};

/**********************************************/
#ifdef BRIDGE
class repeat_node_maker : public node_maker<repeat_node> {
 protected:
  static std::map<std::string, repeat_node_maker*> map_;
 public:
  repeat_node_maker() : node_maker<repeat_node>(NODE_REPEAT) {}
  virtual node* make(host& h,ecf_node* n) {
    return map_[n->type_name()]->make(h, n); }
};

std::map<std::string, repeat_node_maker*> repeat_node_maker::map_;

template <class T, class W>
class repeat_node_builder : public repeat_node_maker {
public:
  repeat_node_builder() { 
    map_[typeid(T).name()] = this; }  
  ~repeat_node_builder() {}
};
#endif
#endif
