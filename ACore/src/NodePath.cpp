/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
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

#include <iostream>
#include <boost/algorithm/string/trim.hpp>

#include "NodePath.hpp"
#include "Str.hpp"

using namespace ecf;

void NodePath::split( const std::string& path, std::vector<std::string>& thePath)
{
	/// The path is of the form "/suite/family/task"
	Str::split(path,thePath,Str::PATH_SEPERATOR());
}

bool NodePath::extractHostPort(const std::string& path,std::string& host, std::string& port)
{
	if (path.empty()) return false;

 	std::vector<std::string>  thePath;
	NodePath::split(path,thePath);

 	if (thePath.empty()) return false;

	//<host>:<port>/suite/family/task
	// first path should be of form <host>:<port>
	size_t colonPos = thePath[0].find_first_of(':');
	if (colonPos == std::string::npos) return false;

	host = thePath[0].substr(0,colonPos);
 	port = thePath[0].substr(colonPos+1);

 	boost::algorithm::trim(host);
 	boost::algorithm::trim(port);
 	if (host.empty()) return false;
 	if (port.empty()) return false;

 	return true;
}


std::string NodePath::createPath(const std::vector<std::string>& vec)
{
	if (vec.empty()) return std::string();

	std::string ret;
	size_t size = vec.size();
	for(size_t i = 0; i < size; i++) {
		ret += Str::PATH_SEPERATOR();
		ret += vec[i];
	}
	return ret;
}

std::string NodePath::removeHostPortFromPath(const std::string& path)
{
 	std::vector<std::string> pathVec;
	NodePath::split(path,pathVec);
	pathVec.erase( pathVec.begin() );
	return NodePath::createPath(pathVec);
}

