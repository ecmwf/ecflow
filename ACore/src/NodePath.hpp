#ifndef NODEPATH_HPP_
#define NODEPATH_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2019 ECMWF.
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
#include <vector>

class NodePath : private boost::noncopyable {
public:
	/// returns the path as a vector of strings, preserving the order
	/// Note: multiple path separator '/' are treated as one separator.
	/// Mimics unix path conventions. hence
	/// '/suite//family///task' will be extracted as 'suite','family','task'
	static void split(const std::string& path,std::vector<std::string>&);

	/// If the path name if form:
	///     //<host>:<port>/suite/family/task
	/// The extract the host and port, return OK if successful
	static bool extractHostPort(const std::string& path, std::string& host, std::string& port);

	/// Given a vector of strings , create a path. "suite","family", returns /suite/family
	static std::string createPath(const std::vector<std::string>&);

	/// Given a path like:   //localhost:3141/suite/family/task
	/// returns              /suite/family/task
	static std::string removeHostPortFromPath(const std::string& path);

private:
 	NodePath();

};
#endif
