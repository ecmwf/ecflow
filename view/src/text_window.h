#ifndef text_window_H
#define text_window_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#ifndef panel_H
#include "panel.h"
#endif

#ifndef find_H
#include "find.h"
#endif

#ifndef tmp_file_H
#include "tmp_file.h"
#endif


class text_window : public find {
public:

	text_window(bool);

	~text_window(); // Change to virtual if base class

	void load(const tmp_file&);
	void clear();

	void print();
	void save();

	virtual Widget text()  = 0;

protected:

	void open_search();
	void open_viewer();

	virtual void search(const char*,bool,bool,bool,bool);

private:

	text_window(const text_window&);
	text_window& operator=(const text_window&);

	tmp_file file_;
	void*    text_map_;
	bool     map_it_;

};

#endif
