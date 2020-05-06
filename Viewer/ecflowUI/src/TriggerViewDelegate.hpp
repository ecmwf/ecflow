//============================================================================
// Copyright 2009-2020 ECMWF.
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

class TriggerViewDelegate : public TreeNodeViewDelegate
{
public:
    explicit TriggerViewDelegate(QWidget *parent=nullptr);

    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const override;

    void setRenderSeparatorLine(bool v) {renderSeparatorLine_ = v;}

protected:
    bool renderSeparatorLine_ {false};
    QPen borderPen_;
};

#endif // TRIGGERVIEWDELEGATE_HPP
