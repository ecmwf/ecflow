//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERYVIEWDELEGATE_HPP_
#define VIEWER_SRC_NODEQUERYVIEWDELEGATE_HPP_

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class ModelColumn;

class NodeQueryViewDelegate : public NodeViewDelegate
{
public:
    explicit NodeQueryViewDelegate(QWidget *parent=0);
    ~NodeQueryViewDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

protected:
    void updateSettings();

    void renderNode(QPainter *painter,const QModelIndex& index,
            							const QStyleOptionViewItemV4& option,QString text) const;

    ModelColumn* columns_;
    QPen borderPen_;

};

#endif /* VIEWER_SRC_NODEQUERYVIEWDELEGATE_HPP_ */
