/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_PlainTextSearchInterface_HPP
#define ecflow_viewer_PlainTextSearchInterface_HPP

#include "AbstractTextEditSearchInterface.hpp"

class QPlainTextEdit;

class PlainTextSearchInterface : public AbstractTextEditSearchInterface {
public:
    PlainTextSearchInterface();
    void setEditor(QPlainTextEdit* e) { editor_ = e; }

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
    QPlainTextEdit* editor_{nullptr};
};

#endif /* ecflow_viewer_PlainTextSearchInterface_HPP */
