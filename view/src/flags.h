#ifndef flags_H
#define flags_H

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#ifndef ecflowview_H
#include "ecflowview.h"
#endif

#include "node.h"

class flags {
public:
	  virtual Boolean eval (node *) = 0;
	  virtual ~flags();
};


class flagNot : public flags
{
	flags *f_;
	virtual Boolean eval (node * n) { return !f_->eval (n); }
public:
	flagNot (flags * f) : f_ (f) { }
	~flagNot() { delete f_; }
};

class flagOr : public flags {
	flags *a_;
	flags *b_;
	virtual Boolean eval (node * n) { return a_->eval (n) || b_->eval (n); }
public:
	flagOr (flags * a, flags * b): a_(a), b_(b) { }
	~flagOr() { delete a_; delete b_; }
};

class flagAnd : public flags {
	flags *a_;
	flags *b_;
	virtual Boolean eval (node * n) { return a_->eval (n) && b_->eval (n); }
public:
	flagAnd (flags * a, flags * b): a_(a), b_(b) { }
	~flagAnd() { delete a_; delete b_; }
};

class typeFlag : public flags { 
	int type_;
public:
	virtual Boolean eval(node *);
	typeFlag(int t) : type_(t) {}
};

class statusFlag : public flags { 
	int status_;
public:
	virtual Boolean eval(node *);
	statusFlag(int t) : status_(t) {}
};

class eventFlag : public flags { 
	int status_;
public:
	virtual Boolean eval(node *);
	eventFlag(int t) : status_(t) {}
};

class procFlag : public flags { 
	typedef Boolean (node::*Proc)() const;
	Proc proc_;
public:
	virtual Boolean eval(node *);
	procFlag(Proc p) : proc_(p) {}
};

class flagAll : public flags {
	virtual Boolean eval (node * n) { return True; }
};

class flagNone : public flags {
	virtual Boolean eval (node * n) { return False; }
};

class showFlag : public flags {
	int show_;
public:
	virtual Boolean eval(node *);
	showFlag(int t) : show_(t) {}
};


class userFlag : public flags {
	int level_;
public:
	virtual Boolean eval(node *);
	userFlag(int t) : level_(t) {}
};

class selectionFlag : public flags {
public:
	virtual Boolean eval(node *);
	selectionFlag() {}
};

#endif
