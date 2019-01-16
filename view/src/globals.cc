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

#include <stdio.h>
#include "globals.h"
#include "str.h"
#include "option.h"
#include "configurator.h"
#include "choice.h"
#include "gui.h"
#include <string>

static option<int>  s0(globals::instance(),"timeout",60);
static option<int>  s1(globals::instance(),"maximum",60);
static option<bool> s3(globals::instance(),"drift",true);
static option<bool> s4(globals::instance(),"poll",true);
static option<bool> s5(globals::instance(),"aborted",true);
static option<bool> s6(globals::instance(),"late",true);
static option<bool> s7(globals::instance(),"restarted",true);
static option<bool> s8(globals::instance(),"new_suites",false);
static option<bool> s9(globals::instance(),"direct_read",true);

static option<bool> s10(globals::instance(),"zombied",false);
static option<bool> s11(globals::instance(),"aliases",false);
static option<bool> s13(globals::instance(),"late_family",false);

static option<bool> e1(globals::instance(),"send_as_alias",false);
static option<int> s12(globals::instance(),"jobfile_length",10000);

// User

static option<choice> u0(globals::instance(),"user_level",0);


globals::globals()
#ifdef alpha
	:configurable("user.default")
#endif
{
}

globals::~globals()
{
}

const char* globals::name() const
{
	return "user.default";
}

globals* globals::instance()
{
	static globals* g = new globals();
	return g;
}


void globals::changed(resource& c)
{
	gui::changed(c);
}

void globals::set_resource(const str& name,int value)
{
	option<int> o(instance(),name,value);
	o = value;
	XECFDEBUG std::cout << "# resource: " << name.c_str() << "   " << value << std::endl;
}

int globals::get_resource(const str& name,int value)
{
	option<int> o(instance(),name,value);
	return o;
}

int globals::user_level()
{
	choice c = u0;
	return c;
}
