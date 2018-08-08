//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMPACTVIEW_HPP
#define COMPACTVIEW_HPP

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

class CompactView : public AbstractNodeView
{

public:
    explicit CompactView(TreeNodeModel* model,QWidget *parent=nullptr);
    ~CompactView() override;

    QRect visualRect(const QModelIndex &index) const override;

protected:   
    void paint(QPainter *painter,const QRegion& region) override;
    void drawRow(QPainter* painter,int start,int xOffset,int &yp,int &itemsInRow,std::vector<int>&);

    void layout(int parentId, bool recursiveExpanding,bool afterIsUninitialized,bool preAllocated) override;

    int itemRow(int item) const override;
    int itemCountInRow(int start) const;
    void rowProperties(int start,int& rowHeight,int &itemsInRow,std::vector<int>& indentVec) const;
    int rowHeight(int start,int forward,int &itemsInRow) const;
    void coordinateForItem(int item,int& itemY,int& itemRowHeight) const;
    int itemAtCoordinate(const QPoint& coordinate) const override;
    int itemAtRowCoordinate(int start,int count,int xPos) const;
    bool isPointInExpandIndicator(int,QPoint) const override {return false;}

    int  firstVisibleItem(int &offset) const override;
    void updateRowCount() override;
    void updateScrollBars() override;
    void updateViewport(const QRect rect) override;

    void adjustWidthInParent(int start);

private:
    int connectorPos(TreeNodeViewItem* item, TreeNodeViewItem* parent) const;
};

#endif // COMPACTVIEW_HPP

