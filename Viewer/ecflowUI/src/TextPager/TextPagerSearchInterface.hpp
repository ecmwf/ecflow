//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHINTERFACE_HPP_
#define VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHINTERFACE_HPP_

#include "AbstractTextEditSearchInterface.hpp"
#include "TextPagerCursor.hpp"

class TextPagerEdit;

class TextPagerSearchInterface : public AbstractTextEditSearchInterface
{
public:
	TextPagerSearchInterface()= default;
	void setEditor(TextPagerEdit* e) {editor_=e;}

	bool findString (QString str, bool highlightAll, QTextDocument::FindFlags findFlags,
					 QTextCursor::MoveOperation move, int iteration,StringMatchMode::Mode matchMode) override;

	void automaticSearchForKeywords(bool) override;
	void refreshSearch() override;
    void clearHighlights() override;
    void disableHighlights() override;
	void enableHighlights() override;
	bool highlightsNeedSearch() override {return false;}
	void gotoLastLine() override;

protected:
	TextPagerCursor::MoveOperation translateCursorMoveOp(QTextCursor::MoveOperation move);
	TextPagerEdit *editor_{nullptr};

};

#endif /* VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHINTERFACE_HPP_ */
