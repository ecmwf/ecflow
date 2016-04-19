/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ArgvCreator.hpp"
#include <sstream>
#include <assert.h>
#include <cstdlib>   // for malloc/free and gcc 4.4.3, not required for gcc 4.2.1
#include <cstring>   // for strcpy and gcc 4.4.3, not required for gcc 4.2.1
using namespace std;

ArgvCreator::ArgvCreator( const std::vector<std::string>& theArgs)
{
	// Create a Argv array
	argc_ = theArgs.size();
	argv_ = (char **) malloc ((argc_ + 1) * sizeof (char *));

	assert(argv_ != NULL);

	for(size_t i = 0; i < theArgs.size(); i++) {
		argv_[i] = (char*) malloc (sizeof (char *) * (theArgs[i].size() + 1)); // allow +1 for \0
	   strcpy (argv_[i], theArgs[i].c_str() );
	}
	argv_[argc_] = NULL;
}

// Destroys argv array
ArgvCreator::~ArgvCreator()
{
	// remove argv array
	for (char** scan = argv_; *scan != NULL; scan++) { free (*scan);}
	free (argv_);
}

std::string ArgvCreator::toString() const
{
	std::stringstream ss;
	for(int i=0; i < argc_; i++) { ss << " arg" << i << ":('" << argv_[i] << "')";}
	return ss.str();
}

