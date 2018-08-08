//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef STANDARDVIEW_HPP
#define STANDARDVIEW_HPP

#include <QAbstractScrollArea>
#include <QBasicTimer>
#include <QItemSelectionModel>
#include <QMap>
#include <QModelIndex>
#include <QPointer>
#include <QSet>
#include <QStyleOptionViewItem>

#include "AbstractNodeView.hpp"

class TreeNodeModel;
class GraphNodeViewItem;
class QStyledItemDelegate;

//Implements a standard tree view (similar to QTreeView) where there is
//one item per row

class StandardView : public AbstractNodeView
{
public:
    explicit StandardView(TreeNodeModel* model,QWidget *parent=nullptr);
    ~StandardView() override;

    QRect visualRect(const QModelIndex &index) const override;

protected:
    void paint(QPainter *painter,const QRegion& region) override;
    void drawRow(QPainter* painter,int start,int xOffset,int &yp,std::vector<int>&);

    void layout(int parentId, bool recursiveExpanding,bool afterIsUninitialized,bool preAllocated) override;

    int itemRow(int item) const override;
    int coordinateForItem(int item) const;
    int itemAtCoordinate(const QPoint& coordinate) const override;
    bool isPointInExpandIndicator(int,QPoint) const override;

    int  firstVisibleItem(int &offset) const override;
    void updateRowCount() override;
    void updateScrollBars() override;
    void updateViewport(const QRect rect) override;

    int expandIndicatorBoxWidth_;
    int expandIndicatorWidth_;

private:
    int connectorPos(TreeNodeViewItem* item) const;
};

#endif // STANDARDVIEW_HPP
