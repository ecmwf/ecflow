/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VariableSearchLine_HPP
#define ecflow_viewer_VariableSearchLine_HPP

#include <QModelIndex>

#include "AbstractSearchLine.hpp"

class QTreeView;

class VariableSearchLine : public AbstractSearchLine {
    Q_OBJECT

public:
    explicit VariableSearchLine(QWidget* parent);
    ~VariableSearchLine() override;
    void setView(QTreeView* view);

public Q_SLOTS:
    void slotFind(QString) override;
    void slotFindNext() override;
    void slotFindPrev() override;
    void slotFindNext(bool) { slotFindNext(); }
    void slotFindPrev(bool) { slotFindPrev(); }
    void slotSortHappened(int, Qt::SortOrder);
    void slotUpdate();
    void slotUpdate(const QModelIndex&, const QModelIndex&);

protected:
    void selectIndex(const QModelIndex& index);
    void clearRequested() {}

    QTreeView* view_;
    QModelIndexList resultItems_;
    int currentResultItem_;
};

#endif /* ecflow_viewer_VariableSearchLine_HPP */
