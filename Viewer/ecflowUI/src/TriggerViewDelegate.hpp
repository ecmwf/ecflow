//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERVIEWDELEGATE_HPP
#define TRIGGERVIEWDELEGATE_HPP

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "TreeNodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class ModelColumn;

class TriggerViewDelegate : public TreeNodeViewDelegate
{
public:
    explicit TriggerViewDelegate(QWidget *parent=0);
    ~TriggerViewDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

protected:
    void updateSettings();

    QPen borderPen_;

};

#endif // TRIGGERVIEWDELEGATE_HPP
