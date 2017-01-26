//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#ifndef translator_H
#include "translator.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifndef choice_H
#include "choice.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>

str translator<int,str>::operator()(int n) 
{
	char buf[80];
	sprintf(buf,"%d",n);
	return str(buf);
}

const str& translator<str,str>::operator()(const str& x) 
{ 
  return x; 
}

int translator<str,int>::operator()(const str& x)
{
	return atol(x.c_str());
}

str translator<long long int,str>::operator()(long long int n) 
{
	char buf[80];
	sprintf(buf,"%lld",n);
	return str(buf);
}

long long int translator<str,long long int>::operator()(const str& x)
{
	return atoll(x.c_str());
}

str translator<uint64_t,str>::operator()(uint64_t n) 
{
	char buf[80];
	sprintf(buf,"%ld",n);
	return str(buf);
}

uint64_t translator<str,uint64_t>::operator()(const str& x)
{
	return atoll(x.c_str());
}

#ifdef WITH_INT128
str translator<uint128,str>::operator()(uint128 n) 
{
	char buf[80];
	sprintf(buf,"%lld",n);
	return str(buf);
}

uint128 translator<str,uint128>::operator()(const str& x)
{
	return atoll(x.c_str());
}
#endif

str translator<ULL,str>::operator()(ULL n) 
{
	char buf[80];
	sprintf(buf,"%ulld",n);
	return str(buf);
}
ULL translator<str,ULL>::operator()(const str& x)
{
	return atoll(x.c_str());
}


const str& translator<bool,str>::operator()(bool n) 
{
	static str t("true");
	static str f("false");

	return n ? t : f;
}

bool translator<str,bool>::operator()(const str& x)
{
	char z = x.c_str()[0];
	return z == 't' || z == 'T' || z == '1';
}

ecf_list* translator<str,ecf_list*>::operator()(const str& x)
{
	ecf_list* l = 0;
	const char* p = x.c_str();
	char word[1024];
	int i = 0;

	while(*p)
	{
		if(*p == ' ') 
		{
			word[i] = 0;
			if(i) {
				ecf_list *z = ecf_node_create(word); 
				z->next = l;
				l = z;
			}
			i = 0;
		}
		else {
			word[i++] = *p;
		}
		p++;
			
	}

	word[i] = 0;
	if(i) {
		ecf_list *z = ecf_node_create(word); 
		z->next = l;
		l = z;
	}
	return l;
}

str translator<ecf_list*,str>::operator()(ecf_list* x)
{
	str s;
	str space = " ";
	while(x)
	{
	  s = s + str(x->name().c_str());		
	  if(x->next) s = s + space;
	  x = x->next;
	}
	return s;
}

#define t_vs std::vector<std::string> 
t_vs translator<str,t_vs >::operator()(const str& x)
{
  t_vs l;
  const char* p = x.c_str();
  char word[1024];
  int i = 0;

  while(*p) {
    if(*p == ' ') {
      word[i] = 0;
      if(i) {
	l.push_back(std::string(word));
      }
      i = 0;
    } else {
      word[i++] = *p;
    }
    p++;			
  }

  word[i] = 0;
  if(i) {
    l.push_back(std::string(word));
  }
  return l;
}

// translator<str, std::vector<std::string> >::operator()(str const&)

str translator<t_vs, str>::operator()(t_vs x) 
{
  str s, space = " ";
  t_vs::iterator j;
  for (j = x.begin(); j != x.end(); ++j) {
    s += str(j->c_str());
    if(j != x.end()) 
      s += space;
  }
  return s;
}

str translator<choice,str>::operator()(const choice& n) 
{
	return translator<int,str>()(n);
}

choice translator<str,choice>::operator()(const str& x)
{
	return choice(atol(x.c_str()));
}
