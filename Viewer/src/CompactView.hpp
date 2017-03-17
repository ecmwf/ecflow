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
#include <QItemSelectionModel>
#include <QMap>
#include <QModelIndex>
#include <QPointer>
#include <QSet>
#include <QStyleOptionViewItem>

class TreeNodeModel;
class GraphNodeViewItem;
class CompactNodeViewDelegate;
class QStyledItemDelegate;

//Struct representing visible items in the view. When an item is collapsed
//all its children will be removed from viewItems.
struct CompactViewItem
{
    CompactViewItem() : parentItem(-1), total(0), expanded(0), hasChildren(0),
                      hasMoreSiblings(0), level(0), width(0), height(0), x(0) {}

    QModelIndex index; //the model index represented by the item.
                       //We remove items whenever the indexes are invalidated
    int parentItem; // parent item index in viewItems
    uint total; // total number of visible children in the view
    uint expanded : 1; //the item expanded
    uint hasChildren : 1; // if the item has children in the model (it is
                          // independent of the expanded/collapsed state)
    uint hasMoreSiblings : 1;
    uint level : 12; // indentation
    uint width: 12;
    uint height : 16;
    uint x: 16;

    int right() const {return x+width;}
    bool isFirstChild() const {return index.row() ==0;}
    bool isLeaf() const {return total == 0;}
};


class CompactView : public QAbstractScrollArea
{
Q_OBJECT

public:
    explicit CompactView(TreeNodeModel* model,QWidget *parent=0);
    ~CompactView();

    QModelIndex currentIndex() const;
    QModelIndexList selectedIndexes() const;
    QModelIndex indexAt(const QPoint &point) const;
    QRect visualRect(const QModelIndex &index) const;
    bool isExpanded(const QModelIndex &index) const;
    void setExpanded(const QModelIndex &index, bool expanded);

public Q_SLOTS:
    void reset();
    void setCurrentIndex(const QModelIndex &index);
    void update(const QModelIndex &index);
    void expand(const QModelIndex &index);
    void collapse(const QModelIndex &index);
    void expandAll();
    void collapseAll();

protected Q_SLOTS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex&,int,int);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    void mousePressEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool viewportEvent(QEvent *event);

    void attachModel();
    void insertItems(const QModelIndex& parent, int);
    void paint(QPainter *painter,const QRegion& region);
    void drawRow(QPainter* painter,int start,int &yp,int &itemsInRow,std::vector<int>&);

    void doItemsLayout(bool hasRemovedItems=false);
    void layout(int parentId, bool recursiveExpanding = false,bool afterIsUninitialized = false);

    int itemCountInRow(int start) const;
    void rowProperties(int start,int& rowHeight,int &itemsInRow,std::vector<int>& indentVec) const;
    int rowHeight(int start,int forward,int &itemsInRow) const;
    void coordinateForItem(int item,int& itemY,int& itemRowHeight) const;
    int itemAtCoordinate(const QPoint& coordinate) const;
    int itemAtRowCoordinate(int start,int count,int xPos) const;

    QModelIndex modelIndex(int i) const;
    int viewIndex(const QModelIndex& index) const;

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command);
    void select(const QModelIndex &topIndex, const QModelIndex &bottomIndex,
                                  QItemSelectionModel::SelectionFlags command);

    QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index,
                                                         const QEvent *event) const;

    QRegion visualRegionForSelection(const QItemSelection &selection) const;

    int  firstVisibleItem(int &offset) const;
    void updateRowCount();
    void updateScrollBars();
    void shiftItems(int start);

    enum ScrollMode {
          ScrollPerItem,
          ScrollPerPixel
      };

    TreeNodeModel* model_;
    CompactNodeViewDelegate* delegate_;

private:
    void expand(int item);
    void collapse(int item);

    void insertViewItems(int pos, int count, const CompactViewItem &viewItem);
    void removeViewItems(int pos, int count);

    int connectorPos(CompactViewItem* item, CompactViewItem* parent) const;

    QStyleOptionViewItem viewOptions() const;

    typedef std::vector<CompactViewItem>::iterator ViewItemIterator;
    ScrollMode verticalScrollMode_;
    mutable std::vector<CompactViewItem> viewItems_;
    int rowCount_;
    mutable int lastViewedItem_;
    QModelIndex root_;
    int topMargin_;
    int leftMargin_;
    int itemGap_;
    int expandButtonSize_;
    enum ExpandButtonMode {Classic,Modern};
    ExpandButtonMode expandButtonMode_;
    QPointer<QItemSelectionModel> selectionModel_;
    QPoint pressedPosition_;
    QPersistentModelIndex pressedIndex_;
    QStyledItemDelegate* itemDelegate_;

    // used when expanding and collapsing items
    QSet<QPersistentModelIndex> expandedIndexes;

    inline bool storeExpanded(const QPersistentModelIndex &idx)
    {
        if(expandedIndexes.contains(idx))
            return false;
        expandedIndexes.insert(idx);
            return true;
    }

    inline bool isIndexExpanded(const QModelIndex &idx) const
    {
        //We first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
        return expandedIndexes.contains(idx);
    }

    inline QItemSelectionModel::SelectionFlags selectionBehaviorFlags() const
    {
        return QItemSelectionModel::NoUpdate;
    }
};

#endif // COMPACTVIEW_HPP

