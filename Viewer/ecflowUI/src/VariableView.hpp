/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VariableView_HPP
#define ecflow_viewer_VariableView_HPP

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>

#include "TreeView.hpp"

class VariableView;

class VariableDelegate : public QStyledItemDelegate {
    friend class VariableView;

public:
    explicit VariableDelegate(QTreeView* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    QPen selectPen_;
    QBrush selectBrush_;
    QBrush selectBrushBlock_;
    QPen borderPen_;
    QPixmap lockPix_;
    int genVarPixId_;
    int shadowGenVarPixId_;
    int mirrorVarPixId_;
    QTreeView* view_;
};

class VariableView : public TreeView {
public:
    explicit VariableView(QWidget* parent = nullptr);

protected:
    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

    VariableDelegate* delegate_;
};

#endif /* ecflow_viewer_VariableView_HPP */
