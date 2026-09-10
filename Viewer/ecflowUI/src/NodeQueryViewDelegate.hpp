/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeQueryViewDelegate_HPP
#define ecflow_viewer_NodeQueryViewDelegate_HPP

#include <string>

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

class ModelColumn;

class NodeQueryViewDelegate : public NodeViewDelegate {
public:
    explicit NodeQueryViewDelegate(QWidget* parent = nullptr);
    ~NodeQueryViewDelegate() override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    void updateSettings() override { updateSettingsInternal(); }

    void
    renderNode(QPainter* painter, const QModelIndex& index, const QStyleOptionViewItem& option, QString text) const;

private:
    void updateSettingsInternal();

    ModelColumn* columns_{nullptr};
    QPen borderPen_;
};

#endif /* ecflow_viewer_NodeQueryViewDelegate_HPP */
