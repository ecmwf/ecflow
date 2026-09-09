/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TextEditSearchLine_HPP
#define ecflow_viewer_TextEditSearchLine_HPP

#include <QTextDocument>
#include <QTimer>

#include "AbstractSearchLine.hpp"

class AbstractTextEditSearchInterface;

class TextEditSearchLine : public AbstractSearchLine {
    Q_OBJECT

public:
    explicit TextEditSearchLine(QWidget* parent);
    ~TextEditSearchLine() override;
    void setSearchInterface(AbstractTextEditSearchInterface*);
    void searchOnReload(bool userClickedReload);
    bool hasInterface() const { return interface_ != nullptr; }

public Q_SLOTS:
    void slotFind(QString) override;
    void slotFindNext() override;
    void slotFindPrev() override;
    void slotFindNext(bool) { slotFindNext(); }
    void slotFindPrev(bool) { slotFindPrev(); }
    void matchModeChanged(int newIndex);
    void on_actionCaseSensitive__toggled(bool) override;
    void on_actionWholeWords__toggled(bool) override;
    void on_actionHighlightAll__toggled(bool) override;
    void slotClose() override;
    void slotHighlight();

protected:
    QTextDocument::FindFlags findFlags();
    bool findString(QString str,
                    bool highlightAll,
                    QTextDocument::FindFlags extraFlags,
                    QTextCursor::MoveOperation move,
                    int iteration);
    void refreshSearch();
    void highlightMatches(QString txt);
    void clearHighlights();
    void disableHighlights();
    bool lastFindSuccessful() { return lastFindSuccessful_; }

    AbstractTextEditSearchInterface* interface_;
    QTimer highlightAllTimer_;
    QColor highlightColour_;
    bool lastFindSuccessful_;
};

#endif /* ecflow_viewer_TextEditSearchLine_HPP */
