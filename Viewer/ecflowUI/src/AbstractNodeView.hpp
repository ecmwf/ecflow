/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_AbstractNodeView_HPP
#define ecflow_viewer_AbstractNodeView_HPP

#include <QAbstractScrollArea>
#include <QBasicTimer>
#include <QItemSelectionModel>
#include <QMap>
#include <QModelIndex>
#include <QPointer>
#include <QSet>
#include <QStyleOptionViewItem>

class Animation;
class TreeNodeModel;
class TreeNodeViewDelegate;

// Struct representing visible items in the view. When an item is collapsed
// all its children will be removed from viewItems.
struct TreeNodeViewItem
{
    TreeNodeViewItem() : expanded(0), hasChildren(0), hasMoreSiblings(0), level(0), width(0), height(0), x(0) {}

    QModelIndex index;     // the model index represented by the item.
                           // We remove items whenever the indexes are invalidated
    int parentItem{-1};    // parent item index in viewItems
    unsigned int total{0}; // total number of visible children in the view
    unsigned int widestInSiblings{0};
    unsigned int expanded : 1;    // the item expanded
    unsigned int hasChildren : 1; // if the item has children in the model (it is
                                  // independent of the expanded/collapsed state)
    unsigned int hasMoreSiblings : 1;
    unsigned int level : 12; // indentation
    unsigned int width : 12;
    unsigned int height : 16;
    unsigned int x : 16;

    int right() const { return x + width; }
    int alignedRight() const { return x + widestInSiblings; }
    bool isFirstChild() const { return index.row() == 0; }
    bool isLeaf() const { return total == 0; }
};

class AbstractNodeView : public QAbstractScrollArea {
    Q_OBJECT

    friend class TreeNodeView;

public:
    explicit AbstractNodeView(TreeNodeModel* model, QWidget* parent = nullptr);
    ~AbstractNodeView() override;

    QModelIndex currentIndex() const;
    QModelIndexList selectedIndexes() const;
    QModelIndex indexAt(const QPoint& point) const;
    virtual QRect visualRect(const QModelIndex& index) const = 0;
    bool isExpanded(const QModelIndex& index) const;
    void setExpanded(const QModelIndex& index, bool expanded);
    void expandAll(const QModelIndex& index);
    void collapseAll(const QModelIndex& index);
    TreeNodeViewDelegate* delegate() const { return delegate_; }

public Q_SLOTS:
    void reset();
    void setCurrentIndex(const QModelIndex& index);
    void update(const QModelIndex& index);
    void expand(const QModelIndex& index);
    void collapse(const QModelIndex& index);
    void expandAll();
    void collapseAll();
    void slotRepaint(Animation*);

protected Q_SLOTS:
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void rowsInserted(const QModelIndex&, int, int);
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void rowsRemoved(const QModelIndex& parent, int start, int end);
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);
    void selectionChangedInView(const QItemSelection& selected, const QItemSelection& deselected);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool viewportEvent(QEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

    void attachModel();
    void insertItems(const QModelIndex& parent, int);

    virtual void paint(QPainter* painter, const QRegion& region) = 0;

    // layout
    void doDelayedWidthAdjustment();
    void doItemsLayout(bool hasRemovedItems = false);
    virtual void layout(int parentId, bool recursiveExpanding, bool afterIsUninitialized, bool preAllocated) = 0;

    // Item lookup
    void scrollTo(const QModelIndex& index);
    virtual int itemRow(int item) const                          = 0;
    virtual int itemAtCoordinate(const QPoint& coordinate) const = 0;
    QModelIndex modelIndex(int i) const;
    int viewIndex(const QModelIndex& index) const;
    virtual bool isPointInExpandIndicator(int, QPoint) const = 0;

    // Expand collapse
    void restoreExpand(const QModelIndex& idx);
    void removeAllFromExpanded(const QModelIndex& index);
    void totalNumOfChildren(const QModelIndex& idx, int& num) const;
    void totalNumOfExpandedChildren(const QModelIndex& idx, int& num) const;
    void toggleExpand(const QModelIndex& idx);
    inline bool isIndexExpanded(const QModelIndex& idx) const {
        // We first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
        return expandedIndexes.contains(idx);
    }

    // navigation
    void navigateNext(const QModelIndex& idx);
    void navigatePrev(const QModelIndex& idx);
    virtual void navigateUp(const QModelIndex& idx)    = 0;
    virtual void navigateDown(const QModelIndex& idx)  = 0;
    virtual void navigateLeft(const QModelIndex& idx)  = 0;
    virtual void navigateRight(const QModelIndex& idx) = 0;
    void navigateHome(const QModelIndex& idx);
    void navigateEnd(const QModelIndex& idx);

    // selection
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command, bool shiftKeyPressed);
    void
    select(const QModelIndex& topIndex, const QModelIndex& bottomIndex, QItemSelectionModel::SelectionFlags command);

    QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event) const;

    QRegion visualRegionForSelection(const QItemSelection& selection) const;

    virtual void updateViewport(const QRect rect) = 0;

    virtual int firstVisibleItem(int& offset) const = 0;
    virtual void updateRowCount()                   = 0;
    virtual void updateScrollBars()                 = 0;
    void adjustWidthInParent(int start);

    void setIndentation(int i) { indentation_ = i; }
    void setExpectedBg(QColor c) { expectedBg_ = c; }
    void setConnectorColour(QColor c) { connectorColour_ = c; }
    void setDrawConnector(bool b) { drawConnector_ = b; }
    void setAutoExpandLeafNode(bool b) { autoExpandLeafNode_ = b; }

    void insertViewItems(int pos, int count, const TreeNodeViewItem& viewItem);
    void removeViewItems(int pos, int count);

    int translation() const;

    enum ScrollMode { ScrollPerItem, ScrollPerPixel };

    TreeNodeModel* model_; // The model
    TreeNodeViewDelegate* delegate_;
    QSet<QPersistentModelIndex> expandedIndexes; // used when expanding and collapsing items

    typedef std::vector<TreeNodeViewItem>::iterator ViewItemIterator;
    ScrollMode verticalScrollMode_;
    mutable std::vector<TreeNodeViewItem> viewItems_;
    int rowCount_;
    int maxRowWidth_;
    QModelIndex root_;
    int topMargin_;
    int leftMargin_;
    int itemGap_;
    int connectorGap_;
    int expandConnectorLength_;
    QPointer<QItemSelectionModel> selectionModel_;
    QColor expectedBg_;
    QColor connectorColour_;
    bool drawConnector_;
    int indentation_;
    bool autoExpandLeafNode_;

private:
    void expand(int item);
    void collapse(int item);
    bool collapseAllCore(const QModelIndex& index);

    mutable int lastViewedItem_;
    QPoint pressedPosition_;
    QPersistentModelIndex pressedIndex_;
    QPersistentModelIndex pressedRefIndex_;
    bool noSelectionOnMousePress_;
    bool autoScroll_;
    QBasicTimer delayedWidth_;

    inline bool storeExpanded(const QPersistentModelIndex& idx) {
        if (expandedIndexes.contains(idx)) {
            return false;
        }
        expandedIndexes.insert(idx);
        return true;
    }

    inline QItemSelectionModel::SelectionFlags selectionBehaviorFlags() const { return QItemSelectionModel::NoUpdate; }
};

#endif /* ecflow_viewer_AbstractNodeView_HPP */
