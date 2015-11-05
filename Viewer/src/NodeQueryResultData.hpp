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

#include <QColor>
#include <QString>

class NodeQueryResultData
{
	friend class NodeQueryEngine;
	friend class NodeQueryResultModel;

public:

	NodeQueryResultData() {}

	NodeQueryResultData(const NodeQueryResultData& d)
	{
		server_=d.server_;
		path_=d.path_;
		state_=d.state_;
		stateCol_=d.stateCol_;
		type_=d.type_;
		attr_=d.attr_;
	}

protected:
	QString server_;
	QString path_;
	QString state_;
	QColor stateCol_;
	QString type_;
	QStringList attr_;
};



#endif /* VIEWER_SRC_NODEQUERYRESULTDATA_HPP_ */
