/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #132 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <sstream>
#include "Version.hpp"
#include "ecflow_version.h"
#include <boost/version.hpp>

namespace ecf {

// ********************************************************************
// IMPORTANT:
// The version number is extracted externally
//   see ACore/doc/extracting_version_number.ddoc
// Hence whenever you change the version, check it can be extracted
// calling the code specified in extracting_version_number.ddoc
// on the command line.
// It is also extracted by python, check build/nightly/build.py
// This minimises the places where we have hard code the version number.
//
// When changing the version change remember to:
//    - re-login into remote system to update ECFLOW_INSTALL_DIR & ECFLOW_PYTHON_INSTALL_DIR
//               required for interactive install
//
// To Install a new version on all the different platforms:
//  . build/nightly/quick_install_.sh
// This is because the definition hold's the last version.
// Hence we must rerun to update the version.
//
// ************************************************************************************
// Use  <minor_number>rc<number> for release candidates, Once release we revert back:
//      0rc1    -> 0
//      10rc3   -> 10
// ************************************************************************************
//
// **Please update file history.ddoc with the changed made for each release ***
// ********************************************************************
#ifdef DEBUG
const std::string Version::TAG = " (debug)";  // Old tag: beta(debug)
#else
const std::string Version::TAG = ""; // Old tag: beta
#endif


// See: http://www.cmake.org/cmake/help/cmake_tutorial.html
// For defining version numbers. This is done is a separate file
// that is then included
std::string Version::description()
{
   std::stringstream ss;
   ss << "Ecflow" << Version::TAG << " version(" << ECFLOW_RELEASE << "." << ECFLOW_MAJOR << "." << ECFLOW_MINOR;

   ss << ") boost(" << Version::boost() << ")";
   std::string the_comp = compiler();
   if (!the_comp.empty()) ss << " compiler(" << the_comp << ")";

   ss << " protocol(";
#if defined(BINARY_ARCHIVE)
   ss << "BINARY_ARCHIVE)";
#elif defined(PORTABLE_BINARY_ARCHIVE)
   ss << "PORTABLE_BINARY_ARCHIVE)";
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   ss << "EOS_PORTABLE_BINARY_ARCHIVE)";
#else
   ss << "TEXT_ARCHIVE)";
#endif

   ss << " Compiled on " << __DATE__ << " " << __TIME__;
   return ss.str();
}

std::string Version::version()
{
   std::stringstream ss;
   ss << "ecflow_" << ECFLOW_RELEASE << "_" << ECFLOW_MAJOR << "_" << ECFLOW_MINOR;
   return ss.str();
}

std::string Version::raw()
{
   std::stringstream ss;
   ss << ECFLOW_RELEASE << "." << ECFLOW_MAJOR << "." << ECFLOW_MINOR;
   return ss.str();
}

std::string Version::boost()
{
   std::stringstream ss;
   ss << BOOST_VERSION / 100000     << "."  // major version
      << BOOST_VERSION / 100 % 1000 << "."  // minor version
      << BOOST_VERSION % 100;               // patch level
   return ss.str();
}

std::string Version::compiler()
{
   std::stringstream ss;
#if defined(_AIX)
   ss << "aix " << __IBMCPP__ ;
#elif defined(HPUX)
   ss << "aCC " << __HP_aCC ;   // type aCC +help, this will show compiler manual, search for Predefined Macros
#else
#if defined(__clang__)
   //  To find the list of defines for clang use:
   //  echo | /usr/local/apps/clang/current/bin/clang++ -dM -E -
   ss << "clang " << __clang_major__ << "." <<  __clang_minor__ ;
#elif defined(__INTEL_COMPILER)
   ss << "intel " <<  __INTEL_COMPILER;
#elif defined(_CRAYC)
   ss << "cray " <<  _CRAYC;
#else
   ss << "gcc " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#endif
#endif
   return ss.str();
}

}
