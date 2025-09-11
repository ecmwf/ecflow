/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "StandardView.hpp"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTimerEvent>
#include <QtAlgorithms>
#include <QtGlobal>

#include "ExpandState.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"

// #define _UI_STANDARDVIEW_DEBUG

StandardView::StandardView(TreeNodeModel* model, QWidget* parent)
    : AbstractNodeView(model, parent),
      expandIndicatorBoxWidth_(20),
      expandIndicatorWidth_(10) {
    // Overwrite some base class values
    drawConnector_ = false;
    indentation_   = 10;

    // This is needed for making the context menu work
    setProperty("view", "tree");

    // we cannot call it from the constructor of the base class
    // because it calls a pure virtual method
    reset();
}

StandardView::~StandardView() = default;

// Creates and initialize the viewItem structure of the children of the element
//  parentId: the items whose children are to be expanded
//  recursiveExpanding: all the children will be expanded
//  afterIsUninitialized: when we recurse from layout(-1) it indicates
//  the items after 'i' are not yet initialized and need not to be moved

void StandardView::layout(int parentId, bool recursiveExpanding, bool afterIsUninitialized, bool preAllocated) {
    // This is the root item.
    if (parentId == -1) {
        rowCount_    = 0;
        maxRowWidth_ = 0;
    }

    QModelIndex parentIndex = (parentId < 0) ? root_ : modelIndex(parentId);

    if (parentId >= 0 && !parentIndex.isValid()) {
        // modelIndex() should never return something invalid for the real items.
        // This can happen if columncount has been set to 0.
        // To avoid infinite loop we stop here.
        return;
    }

    int count      = model_->rowCount(parentIndex);
    bool expanding = true;

    // This is the root item. viewItems must be empty at this point.
    if (parentId == -1) {
        Q_ASSERT(viewItems_.empty());
        Q_ASSERT(preAllocated == false);
        viewItems_.resize(count);
        afterIsUninitialized = true; // It can only be true when we expand from the root!
    }
    // The count of the stored children does not match the actual count
    else if (viewItems_[parentId].total != (uint)count) {
        // Expand
        if (!afterIsUninitialized) {
            // We called expandall for a non-root item. All the new items need must be
            // already instered at this point. This is the duty of the caller routine.
            // const int itemsCount = viewItems_.size();
            // if(recursiveExpanding)
            if (preAllocated) {
                // We called expandAll() for a non-root item. All the needed items need must already be
                // inserted at this point. This is the duty of the caller routine!
                // When layout() is finished we need to adjust the parent of all the items
                // after the insertion position. This is the duty of the caller routine. We
                // have chosen this solution for performance reasons!
            }
            else {
                insertViewItems(parentId + 1, count, TreeNodeViewItem());
            }
        }
        // ExpandAll from the root
        else if (count > 0) {
            viewItems_.resize(viewItems_.size() + count);
        }
    }
    else {
        expanding = false;
    }

    int first              = parentId + 1;
    int last               = 0;
    int children           = 0;
    int level              = (parentId >= 0 ? viewItems_[parentId].level + 1 : 0);
    TreeNodeViewItem* item = nullptr;

#ifdef _UI_STANDARDVIEW_DEBUG
    // if(parentId >=0)
    //     UiLog().dbg() << "layout parent=" << viewItems_[parentId].index.data().toString();
#endif

    // Iterate through the direct children of parent item. At this point all the items
    // needed in the loop below are pre-allocated but not yet initialised.
    for (int i = first; i < first + count; i++) {
        QModelIndex currentIndex = model_->index(i - first, 0, parentIndex);

        last                  = i + children;
        item                  = &viewItems_[last];
        item->parentItem      = parentId;
        item->index           = currentIndex;
        item->hasMoreSiblings = (i < first + count - 1);
        item->level           = level;
        item->expanded        = false;
        item->total           = 0;

        // We compute the size of the item. For attributes we delay the width computation until we
        // actually paint them and we set their width to 300.
        int w = 0, h = 0;
        delegate_->sizeHintCompute(currentIndex, w, h);
        item->width  = w;
        item->height = h;

        int xp = leftMargin_ + expandIndicatorBoxWidth_; // no indentation for the root

        if (parentId >= 0) {
            // item->widestInSiblings=widest;
            xp = viewItems_[parentId].x + indentation_ + expandIndicatorBoxWidth_;
        }
        else {
            // item->widestInSiblings=item->width;
        }

        item->x = xp;

        if (item->alignedRight() > maxRowWidth_) {
            maxRowWidth_ = item->right();
        }

#ifdef _UI_STANDARDVIEW_DEBUG
        // UiLog().dbg() <<  " item=" << currentIndex.data().toString();
#endif
        // We need to expand the item
        if (recursiveExpanding || isIndexExpanded(currentIndex)) {
            if (recursiveExpanding) {
                expandedIndexes.insert(currentIndex);
            }

            item->expanded = true;

#ifdef _UI_STANDARDVIEW_DEBUG
            // UiLog().dbg() <<  " is expanded";
#endif

#ifdef _UI_STANDARDVIEW_DEBUG
            // UiLog().dbg() <<  "  before " <<  item->index.data().toString() <<  " total=" << item->total;
#endif
            // Add the children to the layout
            layout(last, recursiveExpanding, afterIsUninitialized, preAllocated);

            item = &viewItems_[last];

#ifdef _UI_STANDARDVIEW_DEBUG
            // UiLog().dbg() <<  "  after " <<  item->index.data().toString() <<  " total=" << item->total;
#endif
            children += item->total;
            item->hasChildren = item->total > 0;
        }
        else {
            item->hasChildren = model_->hasChildren(currentIndex);
        }
    }

    if (!expanding) {
        return; // nothing changed
    }

#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << " update parent total";
#endif

    int pp = parentId;
    while (pp > -1) {
        viewItems_[pp].total += count;

#ifdef _UI_STANDARDVIEW_DEBUG
        // UiLog().dbg() <<  "  parent=" << viewItems_[pp].index.data().toString() <<
        //                   "  total=" << viewItems_[pp].total;
#endif

        pp = viewItems_[pp].parentItem;
    }
}

// Paint the rows intersecting with the given region
void StandardView::paint(QPainter* painter, const QRegion& region) {
    // Even though the viewport palette is set correctly at the
    // beginning something sets it to another value. Here we try
    // to detect it and correct the palette with the right colour.
    if (expectedBg_.isValid()) {
        QPalette p = viewport()->palette();
        if (p.color(QPalette::Window) != expectedBg_) {
            p.setColor(QPalette::Window, expectedBg_);
            viewport()->setPalette(p);
            viewport()->update();
            expectedBg_ = QColor();
            return;
        }
    }

#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "StandardView::paint -->";
    // UiLog().dbg() << "sizeof(TreeNodeViewItem)=" << sizeof(TreeNodeViewItem);
    // UiLog().dbg() << "region=" << region;
#endif

    int firstVisibleOffset = 0;

    // The first visible item at the top of the viewport
    int firstVisible = firstVisibleItem(firstVisibleOffset);
#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "firstVisible " << firstVisible;
#endif

    if (firstVisible < 0) {
        return;
    }

#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "scrollX" << horizontalScrollBar()->value() << " " << viewport()->width();
#endif

    int xOffset = 0;
    if (horizontalScrollBar()->value() > 0) {
        xOffset = horizontalScrollBar()->value();
        painter->translate(-xOffset, 0);
    }

    const int itemsCount    = viewItems_.size();
    const int viewportWidth = viewport()->width();

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    QVector<QRect> rects = region.rects();
#else
    QVector<QRect> rects;
    for (auto rect : region) {
        rects << rect;
    }
#endif
    QVector<int> drawn;
    bool multipleRects = (rects.size() > 1);

    // Iterate through the rectangles in the region
    for (auto rect : rects) {
        const QRect area = (multipleRects ? QRect(0, rect.y(), viewportWidth, rect.height()) : rect);
#ifdef _UI_STANDARDVIEW_DEBUG
        // UiLog().dbg() << " area=" << area;
#endif
        std::vector<int> indentVec;

        if (drawConnector_) {
            // Initialise indentVec. For each indentation level it tells us if
            // a connector line is to be drawn. Here we scan up to the
            // toplevel item in the firstVisible item's branch.
            indentVec = std::vector<int>(1000, 0);
            if (firstVisible > 0) {
                TreeNodeViewItem* item = &viewItems_[firstVisible];
                int level              = item->level;
                while (item->parentItem >= 0 && level > 0) {
                    TreeNodeViewItem* pt = &viewItems_[item->parentItem];
                    if (item->hasMoreSiblings) {
                        indentVec[item->level] = connectorPos(item);
                    }
                    UI_ASSERT(pt->level == level - 1,
                              "item->parentItem=" << item->parentItem << " pt->level=" << pt->level
                                                  << " level=" << level);
                    item = pt;
                    level--;
                }
            }
        }

        int i = firstVisible;       // the first item at the top of the viewport
        int y = firstVisibleOffset; // we may only see part of the first item

        // start at the top of the viewport  and iterate down through the update area
        for (; i < itemsCount; i++) {
            int itemHeight = viewItems_[i].height;

            if (drawConnector_) {
                // Adjust indentVec
                if (viewItems_[i].hasMoreSiblings) {
                    indentVec[viewItems_[i].level] = connectorPos(&viewItems_[i]);
                }
                else {
                    indentVec[viewItems_[i].level] = 0;
                }
            }

#ifdef _UI_STANDARDVIEW_DEBUG
            // UiLog().dbg() << "row: " << i << " " << itemHeight;
#endif
            // Try to find the first item int the current rect
            if (y + itemHeight > area.top()) {
                break;
            }
            y += itemHeight;
        }

#ifdef _UI_STANDARDVIEW_DEBUG
        // UiLog().dbg() << "y: " << y << " " << area.bottom();
#endif

        // Paint the visible rows in the current rect
        for (; i < itemsCount && y <= area.bottom(); i++) {
            if (!multipleRects || !drawn.contains(i)) {
                // Draw a whole row. It will update y,itemsInRow and indentVec!!
                drawRow(painter, i, xOffset, y, indentVec);

#ifdef _UI_STANDARDVIEW_DEBUG
                // UiLog().dbg() << " row rendered - item=" << i << " y=" << y;
#endif
            }
            else {
                int rh = viewItems_[i].height;
                y += rh;
#ifdef _UI_STANDARDVIEW_DEBUG
                // UiLog().dbg() << " row skipped  - item=" << i << " y=" << y;
#endif
            }

            if (multipleRects) {
                drawn.append(i);
            }
        }
    }
}

// Draw a whole row
void StandardView::drawRow(QPainter* painter, int start, int /*xOffset*/, int& yp, std::vector<int>& indentVec) {
    TreeNodeViewItem* item = &(viewItems_[start]);

    // Get the rowheight
    int rh = item->height;

    // See if there are no multiline items in this row
    bool singleRow = delegate_->isSingleHeight(rh);

#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "  item=" << " " << item->index.data().toString();
#endif

    // Find out the indentation level of the row
    int firstLevel = item->level;

    // Init style option
    QStyleOptionViewItem opt;
    if (selectionModel_->isSelected(item->index)) {
        opt.state |= QStyle::State_Selected;
    }

    int optWidth = 2000;
    if (item->width > optWidth) {
        optWidth = item->width;
    }
    opt.rect = QRect(item->x, yp, optWidth, item->height);

    // We do not render the item if it is outisde the viewport and
    // its parent's right is also outside the viewport. Here we considered that
    // the connector line is always drawn from the child to the parent.
    bool needToDraw = true;
#if 0
    if(item->parentItem >=0)
    {
            if(viewItems_[item->parentItem].right() >= translation() + viewportWidth)
                needToDraw=false;
        }
#endif
    if (needToDraw) {
        // For single rows we center items halfway through the rowHeight
#if 0
        if(singleRow)
        {
           if(item->height < rh)
                {
                    opt.rect.moveTop(yp+(rh-item->height)/2);
                }
        }
#endif

        // QRect vr=visualRect(item->index);
        // painter->fillRect(vr,QColor(120,120,120,120));

        // #ifdef _UI_STANDARDVIEW_DEBUG
        //           UiLog().dbg() << "  optRect=" << opt.rect << " visRect=" << vr;
        // #endif

        // Draw the item with the delegate
        QSize paintedSize;
        delegate_->paintIt(painter, opt, item->index, paintedSize);

        // we have to know if the item width/height is the same that we expected.
        // This can happen when:
        //  -we set a fixed initial width for the item (e.g. for an attribute)
        //   and now we got the real width
        //  -the number of icons or additional extra information
        //   changed for a node (so the width changed)
        //  -the number of lines changed in a multiline label (so the height changed)
        //  -the node becomes submitted
        bool wChanged = paintedSize.width() != item->width;
        bool hChanged = paintedSize.height() != item->height;

        if (wChanged || hChanged) {
            // set new size
            item->width  = paintedSize.width();
            item->height = paintedSize.height();

            if (item->right() > maxRowWidth_) {
                maxRowWidth_ = item->right();
                doDelayedWidthAdjustment();
            }
            else if (hChanged) {
                doDelayedWidthAdjustment();
            }
        }

        // draw expand indicator
        if (item->hasChildren) {
            // We draw a triangle into the middle of the expand indicator box
            float indX = item->x - expandIndicatorBoxWidth_ / 2;
            float indY = yp + item->height / 2;

            // painter->drawRect(QRect(indX-expandIndicatorBoxWidth_/2,indY-item->height/2,
            //                         expandIndicatorBoxWidth_,expandIndicatorBoxWidth_));

            float tw = expandIndicatorWidth_;
            float th = tw / 2. * 0.95;

            QPolygonF shape;
            if (item->expanded) {
                shape << QPointF(indX - tw / 2, indY - 0.2 * th) << QPointF(indX + tw / 2, indY - 0.2 * th)
                      << QPointF(indX, indY + 0.8 * th);
            }
            else {
                shape << QPointF(indX, indY - tw / 2.) << QPointF(indX, indY + tw / 2.) << QPointF(indX + th, indY);
            }

            QPen oriPen = painter->pen();
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(71, 71, 70));
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawPolygon(shape);
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setPen(oriPen);
        }

        // Draw the connector lines
        if (drawConnector_) {
            painter->setPen(connectorColour_);

            int lineX1 = item->x - expandIndicatorBoxWidth_ / 2;
            int lineY  = yp + item->height / 2;

            // For multiline labels the connector shoudl be close to the top
            // not in the middle. We use the parentitem's height to find the proper
            // y position.
            if (!singleRow && item->parentItem >= 0) {
                lineY = yp + viewItems_[item->parentItem].height / 2;
            }

            if (item->hasMoreSiblings) {
                indentVec[item->level] = lineX1;
            }
            else {
                indentVec[item->level] = 0;
            }

            // If not a top level item (e.i. not a server) and a leaf we need to draw
            // a connector line straight to the item
            if (item->parentItem >= 0 && item->hasChildren == 0) {
                int lineX2 = item->x - connectorGap_;

                // First child
                if (item->index.row() == 0) {
                    // horizontal line to the node
                    painter->drawLine(lineX1, lineY, lineX2, lineY);

                    // vertical line to the parent
                    painter->drawLine(lineX1, lineY, lineX1, yp);

                    // line towards the siblings  - downwards
                    if (item->hasMoreSiblings) {
                        painter->drawLine(lineX1, lineY, lineX1, yp + rh);
                    }
                }

                // Child in the middle - has sibling both upwards and downwards
                else if (item->hasMoreSiblings) {
                    // horizontal line to the node
                    painter->drawLine(lineX1, lineY, lineX2, lineY);

                    // vertical line to the parent and sibling
                    painter->drawLine(lineX1, yp, lineX1, yp + rh);
                }

                // The last child - has sibling only upwards
                else {
                    // horizontal line to the node
                    painter->drawLine(lineX1, lineY, lineX2, lineY);

                    // vertical line to the parent
                    painter->drawLine(lineX1, lineY, lineX1, yp);
                }
            }

            // Draw the vertical connector lines for all the levels
            // preceding the first level in the row!
            painter->setPen(connectorColour_);
            for (int j = 0; j < firstLevel; j++) {
                int xp = indentVec[j];
                if (xp != 0) {
                    painter->drawLine(xp, yp, xp, yp + rh);
                }
            }
        }
    }

    yp += rh;
}

int StandardView::connectorPos(TreeNodeViewItem* item) const {
    return item->x - expandIndicatorBoxWidth_ / 2;
}

int StandardView::itemRow(int item) const {
    return item;
}

int StandardView::firstVisibleItem(int& offset) const {
    const int value = verticalScrollBar()->value();
#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "CompactNodeView::firstVisibleItem --> value=" << value;
#endif

    if (verticalScrollMode_ == ScrollPerItem) {
        offset = 0;
        // value is the row number

        if (value < 0 || value >= rowCount_) {
            return -1;
        }

        return value;
    }

    return -1;
}

// This has to be very quick. Called after each collapse/expand.
void StandardView::updateRowCount() {
    rowCount_ = static_cast<int>(viewItems_.size());

#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "CompactNodeView::updateRowCount --> " << rowCount_;
#endif
}

void StandardView::updateScrollBars() {
#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "CompactNodeView::updateScrollBars -->";
#endif

    QSize viewportSize = viewport()->size();
    if (!viewportSize.isValid()) {
        viewportSize = QSize(0, 0);
    }

    if (viewItems_.empty()) {
        // doItemsLayout();
    }

    int itemsInViewport = 0;

    const int itemsCount = viewItems_.size();
    if (itemsCount == 0) {
        return;
    }

    const int viewportHeight = viewportSize.height();
    for (int height = 0, item = itemsCount - 1; item >= 0; item--) {
        // UiLog().dbg() << "item=" << item;
        height += viewItems_[item].height;
        if (height > viewportHeight) {
            break;
        }
        itemsInViewport++;
    }
#ifdef _UI_STANDARDVIEW_DEBUG
    // UiLog().dbg() << "  itemsCount=" << itemsCount << " rowCount=" << rowCount_;
    // UiLog().dbg() << "  itemsInViewport " << itemsInViewport;
#endif

    if (verticalScrollMode_ == ScrollPerItem) {
        if (!viewItems_.empty()) {
            itemsInViewport = qMax(1, itemsInViewport);
        }

        // verticalScrollBar()->setRange(0, itemsCount - itemsInViewport);
        verticalScrollBar()->setRange(0, rowCount_ - itemsInViewport);
        verticalScrollBar()->setPageStep(itemsInViewport);
        verticalScrollBar()->setSingleStep(1);
    }
    else {
        // scroll per pixel
    }

    // Horizontal scrollbar
    if (viewportSize.width() < maxRowWidth_) {
        horizontalScrollBar()->setRange(0, maxRowWidth_ + 10 - viewportSize.width());
        horizontalScrollBar()->setPageStep(viewportSize.width());
        horizontalScrollBar()->setSingleStep(1);
    }
    else {
        horizontalScrollBar()->setRange(0, 0);
    }
}

/*
  Returns the rectangle on the viewport occupied by the item at \a index.
  If the index is not visible or explicitly hidden, the returned rectangle is invalid.
*/
QRect StandardView::visualRect(const QModelIndex& index) const {
    // if (!d->isIndexValid(index) || isIndexHidden(index))
    //     return QRect();

    // d->executePostedLayout();

    int vi = viewIndex(index);
    if (vi < 0) {
        return {};
    }

    int y = coordinateForItem(vi);
    if (y >= 0) {
        // return QRect(viewItems_[vi].x, y, viewItems_[vi].width,rh); //TODO: optimise it
        return {static_cast<int>(viewItems_[vi].x) - 1 - translation(),
                y,
                static_cast<int>(viewItems_[vi].width) + 2,
                static_cast<int>(viewItems_[vi].height)};
    }
    return {};
}

// Returns the viewport y coordinate for  item.
int StandardView::coordinateForItem(int item) const {
    if (verticalScrollMode_ == ScrollPerItem) {
        int offset = 0;
        // firstVisibleItem must always start a row!!!!
        int topViewItemIndex = firstVisibleItem(offset);
        if (item >= topViewItemIndex) {
            // search in the visible area first and continue down
            // ### slow if the item is not visible

            const int itemsCount     = viewItems_.size();
            const int viewportHeight = viewport()->size().height();

            for (int height = 0, viewItemIndex = topViewItemIndex;
                 height <= viewportHeight && viewItemIndex < itemsCount;
                 viewItemIndex++) {
                int h = viewItems_[viewItemIndex].height;
                if (viewItemIndex == item) {
                    return height;
                }
                height += h;
            }
        }
    }

    return -1;
}

// coordinate is in viewport coordinates
int StandardView::itemAtCoordinate(const QPoint& coordinate) const {
    const std::size_t itemCount = viewItems_.size();
    if (itemCount == 0) {
        return -1;
    }

    if (verticalScrollMode_ == ScrollPerItem) {
        // int topRow = verticalScrollBar()->value();

        int offset           = 0;
        int topViewItemIndex = firstVisibleItem(offset);

        if (coordinate.y() >= 0) {
            // the coordinate is in or below the viewport
            int viewItemCoordinate = 0;
            for (std::size_t viewItemIndex = topViewItemIndex; viewItemIndex < itemCount; viewItemIndex++) {
                viewItemCoordinate += viewItems_[viewItemIndex].height;
                if (viewItemCoordinate > coordinate.y()) {
                    // viewItemIndex=itemAtRowCoordinate(viewItemIndex,itemsInRow,coordinate.x()+translation());
                    return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
                }
            }
        }
    }

    return -1;
}

bool StandardView::isPointInExpandIndicator(int item, QPoint p) const {
    const auto itemCount = static_cast<int>(viewItems_.size());
    return item >= 0 && item < itemCount && p.x() > viewItems_[item].x - expandIndicatorBoxWidth_ &&
           p.x() < viewItems_[item].x - 2;
}

void StandardView::updateViewport(const QRect rect) {
    if (rect.right() < viewport()->rect().right()) {
        viewport()->update(rect.adjusted(0, 0, viewport()->rect().right() - rect.right(), 0));
    }
    else {
        viewport()->update(rect);
    }
}

void StandardView::navigateLeft(const QModelIndex& idx) {
    collapse(idx);
}

void StandardView::navigateRight(const QModelIndex& idx) {
    expand(idx);
}

void StandardView::navigateUp(const QModelIndex& idx) {
    navigatePrev(idx);
}

void StandardView::navigateDown(const QModelIndex& idx) {
    navigateNext(idx);
}
