#ifndef translator_H
#define translator_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#include <vector>
// #include <inttypes.h>

#ifdef NO_BOOL
#include "bool.h"
#endif
#include "ecflow.h"
class str;
class choice;
#include "gen_translator.h"

template<> class translator<str,str> {
public:
  const str& operator()(const str& x);
};

template<> class translator<str,bool> {
public:
	bool operator()(const str&);
};

template<> class translator<bool,str> {
public:
	const str& operator()(bool x);
};

template<> class translator<str,int> {
public:
	int operator()(const str&);
};

template<> class translator<int,str> {
public:
	str operator()(int x);
};

template<> class translator<str,long long int> {
public:
  long long int operator()(const str&);
};
template<> class translator<long long int,str> {
public:
	str operator()(long long int x);
};

// #define ULL long long
// #define ULL uint64_t
#define ULL unsigned int
template<> class translator<str,ULL> {public:	ULL operator()(const str&); };
template<> class translator<ULL,str> {public:	str operator()(ULL x); };

template<> class translator<str,uint64_t> {
public:
  uint64_t operator()(const str&);
};
template<> class translator<uint64_t,str> {
public:
	str operator()(uint64_t x);
};

#ifdef WITH_INT128
template<> class translator<uint128,str> {public: str operator()(uint128 x);};
template<> class translator<str,uint128> {public: uint128 operator()(const str&); };
#endif

template<> class translator<str,std::vector<std::string> > {
public:
  std::vector<std::string> operator()(const str&);
};

template<> class translator<std::vector<std::string> ,str> {
public:
	str operator()(std::vector<std::string> );
};

template<> class translator<str,ecf_list*> {
public:
	ecf_list* operator()(const str&);
};

template<> class translator<ecf_list*,str> {
public:
	str operator()(ecf_list*);
};

template<> class translator<str,choice> {
public:
	choice operator()(const str&);
};

template<> class translator<choice,str> {
public:
	str operator()(const choice&);
};

#endif
