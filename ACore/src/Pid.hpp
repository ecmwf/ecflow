#ifndef PID_HPP_
#define PID_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include <string>

class Pid : private boost::noncopyable {
public:
   /// Returns the process pid as a string, otherwise throw exception(std::runtime_error)
   static std::string getpid();

   /// Return the a unique name based on process identifier,otherwise throw exception(std::runtime_error)
   /// returns prefix + _ + getpid();
   static std::string unique_name(const std:: string& prefix);

private:

   Pid()= default;
};

#endif
