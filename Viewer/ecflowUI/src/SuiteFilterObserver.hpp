//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_SUITEFILTEROBSERVER_HPP_
#define VIEWER_SRC_SUITEFILTEROBSERVER_HPP_

class SuiteFilter;

class SuiteFilterObserver
{
public:
      SuiteFilterObserver() = default;
      virtual ~SuiteFilterObserver() = default;

	  virtual void notifyChange(SuiteFilter*)=0;
	  virtual void notifyDelete(SuiteFilter*)=0;
};
#endif /* VIEWER_SRC_SUITEFILTEROBSERVER_HPP_ */
