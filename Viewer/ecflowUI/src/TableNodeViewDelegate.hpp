//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TABLENODEVIEWDELEGATE_HPP
#define TABLENODEVIEWDELEGATE_HPP

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class ModelColumn;

class TableNodeViewDelegate : public NodeViewDelegate
{
 Q_OBJECT
public:
    explicit TableNodeViewDelegate(QWidget *parent=nullptr);
    ~TableNodeViewDelegate() override;

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const override;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const override;

Q_SIGNALS:
    void sizeHintChangedGlobal();

protected:
    void updateSettings() override;

    void renderNode(QPainter *painter,const QModelIndex& index,
                                        const QStyleOptionViewItem& option,QString text) const;

    ModelColumn* columns_;
    QPen borderPen_;
};

#endif // TABLENODEVIEWDELEGATE_HPP

