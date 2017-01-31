//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_CONFIGLISTDELEGATE_HPP_
#define VIEWER_SRC_CONFIGLISTDELEGATE_HPP_

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>

#include "TreeView.hpp"

#include <string>

class ConfigListDelegate : public QStyledItemDelegate
{
public:
    explicit ConfigListDelegate(int,int,QWidget *parent=0);
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;

protected:
    int iconSize_;
    int maxTextWidth_;
    int margin_;
    int gap_;
};

#endif /* VIEWER_SRC_CONFIGLISTDELEGATE_HPP_ */
