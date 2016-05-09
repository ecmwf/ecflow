//============================================================================
// Name        :
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
// Description : This class is used as a helper class
//============================================================================

#include <iostream>
#include "File_r.hpp"

using namespace std;
using namespace ecf;

File_r::File_r(const std::string& file_name) : file_name_(file_name), fp_(file_name.c_str(), std::ios_base::in) {}
File_r::~File_r() { fp_.close(); }

