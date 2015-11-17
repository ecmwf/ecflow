//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERYRESULTDATA_HPP_
#define VIEWER_SRC_NODEQUERYRESULTDATA_HPP_

#include <QString>

class VNode;

class NodeQueryResultData
{
	friend class NodeQueryEngine;
	friend class NodeQueryResultModel;

public:

	NodeQueryResultData() : node_(NULL) {}
	NodeQueryResultData(VNode* node) : node_(node) {}
	NodeQueryResultData(const NodeQueryResultData& d)
	{
		node_=d.node_;
		attr_=d.attr_;
	}

protected:
	VNode* node_;
	QStringList attr_;
};


#endif /* VIEWER_SRC_NODEQUERYRESULTDATA_HPP_ */
