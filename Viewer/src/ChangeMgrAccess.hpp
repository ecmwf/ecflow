//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_CHANGEMGRACCESS_HPP_
#define VIEWER_SRC_CHANGEMGRACCESS_HPP_

#include <QMutex>

class ChangeMgrSingleton;

// -------------------------------------------------------------------------
// ServerDefsAccess - a class to manage access to the server definition tree
// - required for multi-threaded access
// -------------------------------------------------------------------------

class ChangeMgrAccess
{
public:
	explicit ChangeMgrAccess();
	~ChangeMgrAccess();

	ChangeMgrSingleton* changeManager() const;

private:
	static QMutex mutex_;
};


#endif /* VIEWER_SRC_CHANGEMGRACCESS_HPP_ */
