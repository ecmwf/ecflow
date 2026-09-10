/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ConfigListDelegate_HPP
#define ecflow_viewer_ConfigListDelegate_HPP

#include <string>

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>

#include "TreeView.hpp"

class ConfigListDelegate : public QStyledItemDelegate {
public:
    explicit ConfigListDelegate(int, int, QWidget* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    int iconSize_;
    int maxTextWidth_;
    int margin_;
    int gap_;
};

#endif /* ecflow_viewer_ConfigListDelegate_HPP */
