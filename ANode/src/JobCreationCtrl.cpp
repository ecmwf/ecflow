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
// Description :
//============================================================================

#include <iostream>
#include <stdlib.h> // for getenv()

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "JobCreationCtrl.hpp"

namespace fs = boost::filesystem;

void JobCreationCtrl::generate_temp_dir()
{
   if (!getenv("TMPDIR")) throw std::runtime_error("JobCreationCtrl::generate_temp_dir(), The environment variable TMPDIR is not defined");
	tempDirForJobGeneration_ = getenv("TMPDIR");
	tempDirForJobGeneration_ += "/ecf_check_job_creation";
	if (fs::exists(tempDirForJobGeneration_))  fs::remove_all(tempDirForJobGeneration_);
	std::cout << "JobCreationCtrl::generate_temp_dir()  " << tempDirForJobGeneration_ << "\n";
}
