/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
