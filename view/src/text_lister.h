#ifndef text_lister_H
#define text_lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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


#include <stdio.h>

class node;

class text_lister {
public:
  
  text_lister() {}
  
  virtual node* source() const         = 0;
  virtual void push(node*)           = 0;
  // virtual void pusher(node*)           = 0;
  virtual void push(const char*,...) = 0;
  virtual void push(const std::string&) = 0;
  virtual void endline()             = 0;
  virtual void cancel()              = 0;
  
  virtual FILE* file()  { return 0; }

private:

  text_lister(const text_lister&);
  text_lister& operator=(const text_lister&);
};

inline
text_lister& operator<<(text_lister& s,int n) 
{ s.push("%d",n); return s; }

inline
text_lister& operator<<(text_lister& s,char n)
{ s.push("%c",n); return s; }

inline
text_lister& operator<<(text_lister& s,double n)
{ s.push("%g",n); return s; }

inline
text_lister& operator<<(text_lister& s,const char* n)
{ s.push("%s",n); return s; }
inline
text_lister& operator<<(text_lister& s,const std::string n)
{ s.push("%s",n.c_str()); return s; }

inline
text_lister& operator<<(text_lister& s,node* n) 
{ s.push(n); return s; }

inline
text_lister& operator<<(text_lister& s,node& n) 
{ s.push(&n); return s; }

inline
text_lister& operator<<(text_lister& s, void (*proc)(text_lister&)) 
{ proc(s); return s; }

inline void cancel(text_lister& s)  { s.cancel();   }
inline void endl(text_lister& s)    { s.endline();  }


#endif
