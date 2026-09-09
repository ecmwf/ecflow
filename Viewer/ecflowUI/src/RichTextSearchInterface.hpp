/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_RichTextSearchInterface_HPP
#define ecflow_viewer_RichTextSearchInterface_HPP

#include "AbstractTextEditSearchInterface.hpp"

class QTextBrowser;

class RichTextSearchInterface : public AbstractTextEditSearchInterface {
public:
    RichTextSearchInterface();
    void setEditor(QTextBrowser* e) { editor_ = e; }

    bool findString(QString str,
                    bool highlightAll,
                    QTextDocument::FindFlags findFlags,
                    QTextCursor::MoveOperation move,
                    int iteration,
                    StringMatchMode::Mode matchMode) override;

    void automaticSearchForKeywords(bool) override;
    void refreshSearch() override;
    void clearHighlights() override;
    void disableHighlights() override;
    void enableHighlights() override {}
    bool highlightsNeedSearch() override { return true; }
    void gotoLastLine() override;

protected:
    QTextBrowser* editor_{nullptr};
};

#endif /* ecflow_viewer_RichTextSearchInterface_HPP */
