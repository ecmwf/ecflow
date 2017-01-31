#ifndef logsvr_H
#define logsvr_H

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#ifndef ecflowview_H
#include "ecflowview.h"
#endif


#ifndef tmp_file_H
#include "tmp_file.h"
#endif


class logsvr {
public:

  logsvr(std::string host,std::string port);

  ~logsvr(); // Change to virtual if base class
  
  tmp_file getfile(std::string name);
  ecf_dir* getdir(const char* name);
  bool     ok() const { return soc_ >= 0; }

private:

	logsvr(const logsvr&);
	logsvr& operator=(const logsvr&);

	int  soc_;

	void connect(std::string,int);

	std::string host_, port_;
};

inline void destroy(logsvr**) {}

#endif
