/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerViewDelegate_HPP
#define ecflow_viewer_TriggerViewDelegate_HPP

#include <string>

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

#include "TreeNodeViewDelegate.hpp"
#include "VProperty.hpp"

class TriggerViewDelegate : public TreeNodeViewDelegate {
public:
    explicit TriggerViewDelegate(QWidget* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setRenderSeparatorLine(bool v) { renderSeparatorLine_ = v; }

protected:
    bool renderSeparatorLine_{false};
    QPen borderPen_;
};

#endif /* ecflow_viewer_TriggerViewDelegate_HPP */
