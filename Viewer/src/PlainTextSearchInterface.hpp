//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_PLAINTEXTSEARCHINTERFACE_HPP_
#define VIEWER_SRC_PLAINTEXTSEARCHINTERFACE_HPP_

#include "AbstractTextEditSearchInterface.hpp"

class QPlainTextEdit;

class PlainTextSearchInterface : public AbstractTextEditSearchInterface
{
public:
	PlainTextSearchInterface();
	void setEditor(QPlainTextEdit* e) {editor_=e;}

	bool findString (QString str, bool highlightAll, QTextDocument::FindFlags findFlags,
					 QTextCursor::MoveOperation move, int iteration,StringMatchMode::Mode matchMode);

	void automaticSearchForKeywords(bool);
	void refreshSearch();
    void clearHighlights();
    void disableHighlights();
    void enableHighlights() {}
	bool highlightsNeedSearch() {return true;}
	void gotoLastLine();

protected:

	QPlainTextEdit *editor_;

};

#endif /* VIEWER_SRC_PLAINTEXTSEARCHINTERFACE_HPP_ */
