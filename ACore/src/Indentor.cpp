//============================================================================
// Name        : Indentor
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "Indentor.hpp"

namespace ecf {

int Indentor::index_ = 0;


std::ostream& Indentor::indent( std::ostream& os, int char_spaces)
{
	int spaces = index_ * char_spaces;
	for (int i = 0; i != spaces; i++)
		os << " ";
	return os;
}

}
