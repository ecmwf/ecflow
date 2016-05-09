#ifndef TASK_API_HPP_
#define TASK_API_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Task api. i.e the child commands, typically called from the jobs files
//               The two variant api must correspond i.e '--get' and 'get'
//               since this is used by boost program options
//============================================================================
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>

class TaskApi : private boost::noncopyable  {
public:
   /// Used to construct arguments, for argc,argv
	static std::string init(const std::string& process_id);
	static std::string abort(const std::string& reason = "");
	static std::string event(const std::string& eventName);
	static std::vector<std::string> meter(const std::string& meterName,const std::string& metervalue);
	static std::vector<std::string> label(const std::string& label_name, const std::vector<std::string>& labels );
	static std::string complete();
	static std::string wait(const std::string& expression);

 	// Only to be used in Cmd
 	static const char* initArg();
	static const char* abortArg();
	static const char* eventArg();
	static const char* meterArg();
	static const char* labelArg();
	static const char* completeArg();
	static const char* waitArg();
};
#endif
