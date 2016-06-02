#ifndef VERSION_HPP_
#define VERSION_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #15 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Holds the version of ECFlow. Checked against definition file
//               For each major release the major number should be
//               incremented.
// If the parsing comes across a construct it does not recognize, then
// an exception is thrown. i.e if a construct added in release 2.0 of ECF
// is read in by version 1.0 of ECF. The exception should indicate the
// current release
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include <string>

namespace ecf {

class Version : private boost::noncopyable {
public:
	/// Outputs a string of the form:
	/// ECF <tag> version release_.major_.minor_
	static std::string description();

	/// Outputs string of form: ecflow_<release>_<major>_<minor>
	/// This could be used by install
	static std::string version();

   /// Outputs string of form: <release>.<major>.<minor>
   static std::string raw();

private:
	Version();

	static const std::string TAG; // alpha, beta, release

private:
   /// return version of the boost library
   static std::string boost();

   // Return the version of the compiler. Can return empty string
   static std::string compiler();
};
}
#endif
