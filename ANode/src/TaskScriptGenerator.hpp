#ifndef TASKSCRIPTGENERATOR_HPP_
#define TASKSCRIPTGENERATOR_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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

#include <string>
#include <map>
#include <boost/noncopyable.hpp>
class Task;

namespace ecf {

class TaskScriptGenerator : private boost::noncopyable {
public:
   TaskScriptGenerator(const Task*);

   void generate(const std::map<std::string,std::string>& override);

private:
   void generate_head_file() const;
   void generate_tail_file() const;
   std::string getDefaultTemplateEcfFile() const;

private:
   const Task* task_;
   bool is_dummy_task_;
   std::string ecf_files_;
   std::string ecf_home_;
   std::string ecf_include_;
};

}
#endif
