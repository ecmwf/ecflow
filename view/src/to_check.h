#ifndef to_check_H
#define to_check_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/


#include "node_alert.h"

class node;

class to_check : public node_alert<to_check> {
public:
  to_check();

   ~to_check(); // Change to virtual if base class

private:

	to_check(const to_check&);
	to_check& operator=(const to_check&);

	virtual bool keep(node* n);

};

inline void destroy(to_check**) {}

#endif
