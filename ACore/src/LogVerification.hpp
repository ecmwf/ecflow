#ifndef LOG_VERIFICATION_HPP_
#define LOG_VERIFICATION_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
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
#include <vector>
#include <string>
#include <boost/noncopyable.hpp>

namespace ecf {

class LogVerification : private boost::noncopyable {
public:

    /// Given a log file, extract in order. The node_path and the state
    static bool extractNodePathAndState(const std::string& logfile,
                                        std::vector< std::pair<std::string,std::string> >& pathStateVec,
                                        std::string& errorMsg);

	/// Will compare the input log file, with gold reference.
 	/// Will compare the node state changes only
    /// Compensate for states that are scheduler dependent
 	static bool compareNodeStates( const std::string& logfile, const std::string& goldenRefLogFile, std::string& errormsg);

private:
	LogVerification() {}
};
}
#endif
