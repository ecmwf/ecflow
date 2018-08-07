//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_ABSTRACTTEXTEDITSEARCHINTERFACE_HPP_
#define VIEWER_SRC_ABSTRACTTEXTEDITSEARCHINTERFACE_HPP_

#include "StringMatchMode.hpp"
#include "VProperty.hpp"

#include <QColor>
#include <QTextDocument>
#include <QTextCursor>

class AbstractTextEditSearchInterface
{
public:
	AbstractTextEditSearchInterface();
    virtual ~AbstractTextEditSearchInterface() = default;

	virtual bool findString (QString str, bool highlightAll, QTextDocument::FindFlags findFlags,
							 QTextCursor::MoveOperation move, int iteration,StringMatchMode::Mode matchMode)=0;
	virtual void automaticSearchForKeywords(bool)=0;
	virtual void refreshSearch()=0;
    virtual void clearHighlights()=0;
    virtual void disableHighlights()=0;
	virtual void enableHighlights()=0;
	virtual bool highlightsNeedSearch()=0;
	virtual void gotoLastLine()=0;

protected:
	static QColor highlightColour_;
	VProperty *vpPerformAutomaticSearch_;
	VProperty *vpAutomaticSearchMode_;
	VProperty *vpAutomaticSearchText_;
	VProperty *vpAutomaticSearchFrom_;
	VProperty *vpAutomaticSearchCase_;
};


#endif /* VIEWER_SRC_ABSTRACTTEXTEDITSEARCHINTERFACE_HPP_ */
