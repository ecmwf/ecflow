/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AbstractSearchLine_HPP
#define ecflow_viewer_AbstractSearchLine_HPP

#include <QWidget>

#include "ui_SearchLineWidget.h"

class AbstractSearchLine : public QWidget, protected Ui::SearchLineWidget {
    Q_OBJECT

public:
    explicit AbstractSearchLine(QWidget* parent = nullptr);
    ~AbstractSearchLine() override;
    virtual void clear();
    virtual bool isEmpty();
    void selectAll();
    void setConfirmSearch(bool);
    bool confirmSearch() const { return confirmSearch_; }
    QString confirmSearchText() const;

    bool caseSensitive() { return caseSensitive_; }
    bool wholeWords() { return wholeWords_; }
    bool highlightAll() { return highlightAll_; }

public Q_SLOTS:
    virtual void slotFind(QString) = 0;
    virtual void slotFindNext()    = 0;
    virtual void slotFindPrev()    = 0;
    virtual void slotClose();
    virtual void on_actionCaseSensitive__toggled(bool);
    virtual void on_actionWholeWords__toggled(bool);
    virtual void on_actionHighlightAll__toggled(bool);

Q_SIGNALS:
    void visibilityChanged();

protected:
    void updateButtons(bool);
    void toDefaultState();
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

    bool status_{true};
    bool caseSensitive_{false};
    bool wholeWords_{false};
    bool highlightAll_{false};
    StringMatchMode matchMode_;

    QBrush oriBrush_;
    QBrush redBrush_;
    QBrush greenBrush_;

    bool confirmSearch_{false};
};

#endif /* ecflow_viewer_AbstractSearchLine_HPP */
