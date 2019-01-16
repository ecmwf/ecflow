//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef RichTextSearchInterface_HPP
#define RichTextSearchInterface_HPP

#include "AbstractTextEditSearchInterface.hpp"

class QTextBrowser;

class RichTextSearchInterface : public AbstractTextEditSearchInterface
{
public:
    RichTextSearchInterface();
    void setEditor(QTextBrowser* e) {editor_=e;}

    bool findString(QString str, bool highlightAll, QTextDocument::FindFlags findFlags,
                     QTextCursor::MoveOperation move, int iteration,StringMatchMode::Mode matchMode);

    void automaticSearchForKeywords(bool);
    void refreshSearch();
    void clearHighlights();
    void disableHighlights();
    void enableHighlights() {}
    bool highlightsNeedSearch() {return true;}
    void gotoLastLine();

protected:

    QTextBrowser *editor_;
};

#endif // RichTextSearchInterface_HPP
