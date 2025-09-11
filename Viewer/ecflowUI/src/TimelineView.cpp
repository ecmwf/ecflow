/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TimelineView.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>
#include <QtGlobal>

#include "ActionHandler.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "TimelineData.hpp"
#include "TimelineHeaderView.hpp"
#include "TimelineInfoWidget.hpp"
#include "TimelineModel.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"
#include "VNState.hpp"
#include "VSettings.hpp"
#include "ViewerUtil.hpp"

#define _UI_TimelineView_DEBUG

static std::vector<std::string> propVec;

//======================================================================
//
// TimelineDelegate
//
//======================================================================

TimelineDelegate::TimelineDelegate(TimelineModel* model, QWidget* parent)
    : QStyledItemDelegate(parent),
      model_(model),
      fm_(QFont()),
      borderPen_(QPen(QColor(216, 216, 216))),
      topPadding_(2),
      bottomPadding_(2),
      submittedMaxDuration_(-1),
      activeMaxDuration_(-1),
      submittedMaxTextWidth_(-1),
      activeMaxTextWidth_(-1) {
    Q_ASSERT(model_);

    fm_ = QFontMetrics(font_);

    // Property
    if (propVec.empty()) {
        propVec.emplace_back("view.table.font");

        // Base settings
        // addBaseSettings(propVec);
    }

    prop_ = new PropertyMapper(propVec, this);

    updateSettings();
}

TimelineDelegate::~TimelineDelegate() {
    delete prop_;
}

void TimelineDelegate::notifyChange(VProperty*) {
    updateSettings();
}

void TimelineDelegate::updateSettings() {
    if (VProperty* p = prop_->find("view.table.font")) {
        auto newFont = p->value().value<QFont>();

        if (font_ != newFont) {
            font_ = newFont;
            fm_   = QFontMetrics(font_);
            Q_EMIT sizeHintChangedGlobal();
        }
    }

    // Update the settings handled by the base class
    // updateBaseSettings();
}

QSize TimelineDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    return {size.width(), fm_.height() + topPadding_ + bottomPadding_};
    return size;
}

void TimelineDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    // Background
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);
    bool selected = option.state & QStyle::State_Selected;

    // Save painter state
    painter->save();

    if (index.column() == TimelineModel::TimelineColumn) {
        renderTimeline(painter, option, index.data().toInt());
    }
    else if (index.column() == TimelineModel::SubmittedDurationColumn) {
        renderSubmittedDuration(painter, option, index);
        QRect bgRect = option.rect;
        painter->setPen(borderPen_);
        painter->drawLine(
            bgRect.x() + bgRect.width() - 1, bgRect.top(), bgRect.x() + bgRect.width() - 1, bgRect.bottom());
    }
    else if (index.column() == TimelineModel::ActiveDurationColumn) {
        renderActiveDuration(painter, option, index);
    }

    // The path column
    else {
        QRect bgRect = option.rect;
        painter->fillRect(bgRect, QColor(244, 244, 245));
        painter->setPen(borderPen_);
        painter->drawLine(
            bgRect.x() + bgRect.width() - 1, bgRect.top(), bgRect.x() + bgRect.width() - 1, bgRect.bottom());

        QString text = index.data(Qt::DisplayRole).toString();
        // QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
        QRect textRect = bgRect.adjusted(2, topPadding_, -3, -bottomPadding_);
        text           = fm_.elidedText(text, Qt::ElideMiddle, textRect.width());
        textRect.setWidth(ViewerUtil::textWidth(fm_, text));
        painter->setFont(font_);
        painter->setPen(Qt::black);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

        if (selected) {
            QRect sr = textRect;
            sr.setX(option.rect.x() + 1);
            sr.setY(option.rect.y() + 2);
            sr.setBottom(option.rect.bottom() - 2);
            sr.setRight(textRect.x() + textRect.width() + 1);
            painter->setPen(QPen(Qt::black, 2));
            painter->drawRect(sr);
        }
    }

    // Render the horizontal border for rows. We only render the top border line.
    // With this technique we miss the bottom border line of the last row!!!
    // QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect = option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(), bgRect.topRight());

    painter->restore();
}

void TimelineDelegate::renderTimeline(QPainter* painter, const QStyleOptionViewItem& option, int row) const {
    if (row < 0) {
        return;
    }

    TimelineData* data = model_->data();
    if (!data) {
        return;
    }

    int leftEdge  = option.rect.x();
    int rightEdge = option.rect.x() + option.rect.width();
    int xpPrev    = leftEdge - 2;
    int xpNext    = rightEdge + 2;

    const TimelineItem& item = data->items()[row];
    int extendedRight        = -1;

    for (size_t i = 0; i < item.size(); i++) {
        bool hasRect = false;
        bool hasGrad = false;
        bool lighter = false;
        int xpLeft = 0, xpRight = 0;
        QColor fillCol;

        int xp = (i == 0) ? (timeToPos(option.rect, item.start_[i])) : xpNext;

        if (i < item.size() - 1) {
            xpNext = timeToPos(option.rect, item.start_[i + 1]);
        }
        else {
            xpNext = rightEdge + 2;
        }

        if (xp >= leftEdge && xp < rightEdge) {
            if (i > 0 && xpPrev < leftEdge) {
                if (VNState* vn = VNState::find(item.status_[i - 1])) {
                    xpLeft  = leftEdge;
                    xpRight = xp;
                    fillCol = vn->colour();
                    lighter = (vn->name() == "complete");
                    drawCell(painter,
                             QRect(xpLeft, option.rect.y(), xpRight - xpLeft + 1, option.rect.height() - 1),
                             fillCol,
                             true,
                             lighter);
                }
            }

            if (VNState* vn = VNState::find(item.status_[i])) {
                hasRect = true;
                hasGrad = true;
                xpLeft  = xp;
                xpRight = (xpNext <= rightEdge) ? xpNext : rightEdge;
                lighter = (vn->name() == "complete");
                fillCol = vn->colour();
            }
        }
        else if (i > 0 && xp >= rightEdge && xpPrev < leftEdge) {
            if (VNState* vn = VNState::find(item.status_[i - 1])) {
                hasRect = true;
                xpLeft  = leftEdge;
                xpRight = rightEdge;
                lighter = (vn->name() == "complete");
                fillCol = vn->colour();
            }
        }
        else if (xp <= leftEdge && i == item.size() - 1) {
            if (VNState* vn = VNState::find(item.status_[i])) {
                hasRect = true;
                xpLeft  = leftEdge;
                xpRight = rightEdge;
                lighter = (vn->name() == "complete");
                fillCol = vn->colour();
            }
        }

        if (hasRect) {
            // small rects are extended and use no gradient
            if (xpRight - xpLeft < 5) {
                hasGrad       = false;
                xpRight       = xpLeft + 4;
                extendedRight = xpRight;
            }
            else {
                if (extendedRight > xpLeft) {
                    xpLeft = extendedRight;
                }
                extendedRight = -1;
            }

            drawCell(painter,
                     QRect(xpLeft, option.rect.y(), xpRight - xpLeft + 1, option.rect.height() - 1),
                     fillCol,
                     hasGrad,
                     lighter);

            // painter->fillRect(QRect(xpLeft,option.rect.y(),
            //        xpRight-xpLeft+1,option.rect.height()-1),fillBrush);
        }

        if (xp >= rightEdge) {
            break;
        }

        xpPrev = xp;
    }

    // The initial filled rect (we will adj }ust its  width)
    // QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);
}

void TimelineDelegate::renderSubmittedDuration(QPainter* painter,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index) const {
    if (submittedMaxDuration_ <= 0) {
        return;
    }

    //    int val=index.data().toInt();
    //    if(val >= 0)
    //    {
    if (VNState* vn = VNState::find("submitted")) {
        if (useMeanDuration_) {
            QVariant va      = index.data(TimelineModel::MeanDurationRole);
            QVariantList lst = va.toList();
            if (lst.count() == 2) {
                float meanVal = lst[0].toFloat();
                renderDuration(
                    painter, meanVal, submittedMaxDuration_, vn->colour(), option.rect, submittedMaxTextWidth_);
            }
        }
        else {
            int val = index.data().toInt();
            if (val >= 0) {
                renderDuration(painter, val, submittedMaxDuration_, vn->colour(), option.rect, submittedMaxTextWidth_);
            }
        }

        //            float meanVal=-1;
        //            int num=0;
        //            QVariant va=index.data(TimelineModel::MeanDurationRole);
        //            if(va.type() == QVariant::List)
        //            {
        //                QVariantList lst=va.toList();
        //                if(lst.count() == 2)
        //                {
        //                    meanVal=lst[0].toFloat();
        //                    num=lst[1].toInt();
        //                    renderDuration(painter,val, meanVal, submittedMaxDuration_, num,
        //                    vn->colour(),option.rect,submittedMaxTextWidth_);
        //                }
        //            }
        //        }
    }
}

void TimelineDelegate::renderActiveDuration(QPainter* painter,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
    if (activeMaxDuration_ <= 0) {
        return;
    }

    if (VNState* vn = VNState::find("active")) {
        if (useMeanDuration_) {
            QVariant va      = index.data(TimelineModel::MeanDurationRole);
            QVariantList lst = va.toList();
            if (lst.count() == 2) {
                float meanVal = lst[0].toFloat();
                renderDuration(painter, meanVal, activeMaxDuration_, vn->colour(), option.rect, activeMaxTextWidth_);
            }
        }
        else {
            int val = index.data().toInt();
            if (val >= 0) {
                renderDuration(painter, val, activeMaxDuration_, vn->colour(), option.rect, activeMaxTextWidth_);
            }
        }
    }

    //    int val=index.data().toInt();
    //    if(val >= 0)
    //    {
    //        if(VNState* vn=VNState::find("active"))
    //        {
    //            float meanVal=-1;
    //            int num=0;
    //            QVariant va=index.data(TimelineModel::MeanDurationRole);
    //            if(va.type() == QVariant::List)
    //            {
    //                QVariantList lst=va.toList();
    //                if(lst.count() == 2)
    //                {
    //                    meanVal=lst[0].toFloat();
    //                    num=lst[1].toInt();
    //                    renderDuration(painter,val, meanVal, activeMaxDuration_, num,
    //                    vn->colour(),option.rect,activeMaxTextWidth_);
    //                }
    //            }
    //        }
    //    }
}

void TimelineDelegate::drawCell(QPainter* painter, QRect r, QColor fillCol, bool hasGrad, bool lighter) const {
    if (lighter) {
        fillCol = fillCol.lighter(130);
        fillCol.setAlpha(110);
    }
    else {
        fillCol = fillCol.darker(110);
    }

    QBrush fillBrush(fillCol);
    if (hasGrad) {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0, 0);
        gr.setFinalStop(1, 0);
        gr.setColorAt(0, fillCol);
        fillCol.setAlpha(lighter ? 40 : 128);
        gr.setColorAt(1, fillCol);
        fillBrush = QBrush(gr);
    }

    painter->fillRect(r, fillBrush);
}

void TimelineDelegate::renderDuration(QPainter* painter,
                                      int val,
                                      float meanVal,
                                      int maxVal,
                                      int num,
                                      QColor col,
                                      QRect rect,
                                      int maxTextW) const {
    if (maxTextW <= 0) {
        return;
    }

    if (rect.width() <= maxTextW) {
        maxTextW = 0;
    }

    int len = (static_cast<float>(val) / static_cast<float>(maxVal)) * static_cast<float>(rect.width() - maxTextW);

    // bar
    QRect barRect(rect);
    barRect.setWidth(len);
    barRect.adjust(0, 1, -1, -1);
    int right = barRect.x() + barRect.width();

    QString text;
    QRect textRect;
    QString percentText;
    QRect percentRect;

    if (maxTextW > 0) {
        text     = ViewerUtil::formatDuration(val);
        textRect = rect;
        textRect.setX(right + 5);
        textRect.setWidth(ViewerUtil::textWidth(fm_, text));
        right = textRect.x() + textRect.width();
    }

    // diff to mean
    if (meanVal > 0.) {
        int percent = 100.0 * static_cast<float>(val - meanVal) / meanVal;

        if (percent > 0) {
            // unicode U+2191 arrow up
            percentText += QChar(8593);
            // painter->setPen(Qt::red);
        }
        else {
            // unicode U+2193 arrow up
            percentText += QChar(8595);
            // painter->setPen(QColor(11,111,34));
            percent *= -1;
        }

        percentText += QString::number(percent) + "%[" + QString::number(num) + "]";

        percentRect = rect;
        percentRect.setX(right + 5);
        percentRect.setWidth(ViewerUtil::textWidth(fm_, percentText));
        right = percentRect.x() + percentRect.width();
    }

    const bool setClipRect = right > rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(rect.adjusted(0, 0, 0, -2));
    }

    // bar
    painter->fillRect(barRect, col);

    // value
    if (!text.isEmpty()) {
        painter->setPen(QColor(100, 100, 100));
        painter->drawText(textRect, text, Qt::AlignLeft | Qt::AlignVCenter);
    }

    // value
    if (!percentText.isEmpty()) {
        painter->setPen(QColor(100, 100, 100));
        painter->drawText(percentRect, percentText, Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (setClipRect) {
        painter->restore();
    }
}

void TimelineDelegate::renderDuration(QPainter* painter, int val, int maxVal, QColor col, QRect rect, int maxTextW)
    const {
    if (maxTextW <= 0) {
        return;
    }

    if (rect.width() <= maxTextW) {
        maxTextW = 0;
    }

    int len = (static_cast<float>(val) / static_cast<float>(maxVal)) * static_cast<float>(rect.width() - maxTextW);

    // bar
    QRect barRect(rect);
    barRect.setWidth(len);
    barRect.adjust(0, 1, -1, -1);
    int right = barRect.x() + barRect.width();

    QString text;
    QRect textRect;
    QString percentText;

    if (maxTextW > 0) {
        text     = ViewerUtil::formatDuration(val);
        textRect = rect;
        textRect.setX(right + 5);
        textRect.setWidth(ViewerUtil::textWidth(fm_, text));
        right = textRect.x() + textRect.width();
    }

    const bool setClipRect = right > rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(rect.adjusted(0, 0, 0, -2));
    }

    // bar
    painter->fillRect(barRect, col);

    // value
    if (!text.isEmpty()) {
        painter->setPen(QColor(100, 100, 100));
        painter->drawText(textRect, text, Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (setClipRect) {
        painter->restore();
    }
}

int TimelineDelegate::timeToPos(QRect r, unsigned int time) const {
    unsigned int start = TimelineItem::fromQDateTime(startDate_);
    unsigned int end   = TimelineItem::fromQDateTime(endDate_);

    if (time < start) {
        return r.x() - 2;
    }

    if (time >= end) {
        return r.x() + r.width() + 2;
    }

    if (start >= end) {
        return r.x() - 2;
    }

    return r.x() + static_cast<float>(time - start) * static_cast<float>(r.width()) / static_cast<float>((end - start));
}

void TimelineDelegate::setStartDate(QDateTime t) {
    startDate_ = t;
}

void TimelineDelegate::setEndDate(QDateTime t) {
    endDate_ = t;
}

void TimelineDelegate::setPeriod(QDateTime t1, QDateTime t2) {
    startDate_ = t1;
    endDate_   = t2;
}

void TimelineDelegate::setMaxDurations(int submittedDuration, int activeDuration) {
    submittedMaxDuration_ = submittedDuration;
    activeMaxDuration_    = activeDuration;

    submittedMaxTextWidth_ = getDurationMaxTextWidth(submittedMaxDuration_);
    activeMaxTextWidth_    = getDurationMaxTextWidth(activeMaxDuration_);
}

int TimelineDelegate::getDurationMaxTextWidth(int duration) const {
    QString txt;

    if (duration < 60) {
        txt = " 59s";
    }
    else if (duration < 3600) {
        txt = " 59m 59s";
    }
    else if (duration < 3600 * 24) {
        txt = " 23h 59m 59s";
    }
    else if (duration < 86400 * 10) {
        txt = " 9d 23h 59m 59s";
    }
    else if (duration < 86400 * 100) {
        txt = " 99d 23h 59m 59s";
    }
    else {
        txt = " 999d 23h 59m 59s";
    }

    return ViewerUtil::textWidth(fm_, txt + " A999%[999] ");
}

//======================================================================
//
// TimelineView
//
//======================================================================

TimelineView::TimelineView(TimelineSortModel* model, QWidget* parent)
    : QTreeView(parent),
      model_(model),
      header_(nullptr),
      headerBeingAdjusted_(false),
      needItemsLayout_(false),
      prop_(nullptr),
      setCurrentIsRunning_(false),
      viewMode_(TimelineMode),
      durationColumnWidthInitialised_(false) {
    setObjectName("view");
    // setProperty("style","nodeView");
    // setProperty("view","table");
    setProperty("log", "1");
    setRootIsDecorated(false);

    // We enable sorting but do not want to perform it immediately
    // setSortingEnabledNoExec(true);

    // we need a custom sorting
    setSortingEnabled(false);

    setAutoScroll(true);
    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
    setAlternatingRowColors(false);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    // setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    QTreeView::setModel(model_);

    //!!!!We need to do it because:
    // The background colour between the views left border and the nodes cannot be
    // controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    // colour from the palette. Here we set this to transparent so that Qt could leave
    // this area empty and we fill it appropriately in our delegate.
    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(128, 128, 128, 0));
    setPalette(pal);

    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotContextMenu(const QPoint&)));

    // Selection
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClickItem(const QModelIndex)));

    // Header
    header_ = new MainTimelineHeader(this);
    setHeader(header_);

    connect(header_,
            SIGNAL(periodSelected(QDateTime, QDateTime)),
            this,
            SLOT(periodSelectedInHeader(QDateTime, QDateTime)));

    connect(header_,
            SIGNAL(periodBeingZoomed(QDateTime, QDateTime)),
            this,
            SIGNAL(periodBeingZoomed(QDateTime, QDateTime)));

    adjustHeader();

    // When the sort model is invalidated it might change the column visibility
    // which we need to readjust!
    connect(model_, SIGNAL(invalidateCalled()), this, SLOT(adjustHeader()));

    // Create delegate to the view
    delegate_ = new TimelineDelegate(model_->tlModel(), this);
    setItemDelegate(delegate_);

    connect(delegate_, SIGNAL(sizeHintChangedGlobal()), this, SLOT(slotSizeHintChangedGlobal()));

    // Properties
    std::vector<std::string> propVec;
    propVec.emplace_back("view.table.background");
    prop_ = new PropertyMapper(propVec, this);

    // Initialise bg
    adjustBackground(prop_->find("view.table.background")->value().value<QColor>());
}

TimelineView::~TimelineView() {
    delete prop_;
}

// Enable sorting without actually performing it!!!
void TimelineView::setSortingEnabledNoExec(bool /*b*/) {
#if 0
    if(b)
    {
        model_->setSkipSort(true);
        setSortingEnabled(true);
        model_->setSkipSort(false);
    }
    else
    {
        setSortingEnabled(false);
    }
#endif
}

// Collects the selected list of indexes
QModelIndexList TimelineView::selectedList() {
    QModelIndexList lst;
    Q_FOREACH (QModelIndex idx, selectedIndexes()) {
        if (idx.column() == 0) {
            lst << idx;
        }
    }
    return lst;
}

// reimplement virtual function from QTreeView - called when the selection is changed
void TimelineView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    QTreeView::selectionChanged(selected, deselected);

#if 0
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        VInfo_ptr info=model_->nodeInfo(lst.front());
        if(info && !info->isEmpty())
        {
    #ifdef _UI_TimelineView_DEBUG
            UiLog().dbg() << "TimelineView::selectionChanged --> emit=" << info->path();
    #endif
            Q_EMIT selectionChanged(info);
        }
    }
    QTreeView::selectionChanged(selected, deselected);
#endif
}

VInfo_ptr TimelineView::currentSelection() {
#if 0
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        return model_->nodeInfo(lst.front());
    }
#endif
    return {};
}

void TimelineView::setCurrentSelection(VInfo_ptr /*info*/) {
#if 0
    //While the current is being selected we do not allow
    //another setCurrent call go through
    if(setCurrentIsRunning_)
        return;

    setCurrentIsRunning_=true;
    QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
    #ifdef _UI_TimelineView_DEBUG
        if(info)
            UiLog().dbg() << "TimelineView::setCurrentSelection --> " <<  info->path();
    #endif
        setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
#endif
}

void TimelineView::slotDoubleClickItem(const QModelIndex& idx) {
    showDetails(idx);
}

void TimelineView::slotContextMenu(const QPoint& position) {
    QModelIndexList lst = selectedList();
    // QModelIndex index=indexAt(position);
    QPoint scrollOffset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    handleContextMenu(indexAt(position), lst, mapToGlobal(position), position + scrollOffset, this);
}

void TimelineView::handleContextMenu(QModelIndex indexClicked,
                                     QModelIndexList /*indexLst*/,
                                     QPoint globalPos,
                                     QPoint /*widgetPos*/,
                                     QWidget* /*widget*/) {
    if (!indexClicked.isValid()) {
        return;
    }

    auto* menu = new QMenu(this);

    auto* ac = new QAction(this);
    ac->setText(tr("Show details"));
    QFont fBold;
    fBold.setBold(true);
    ac->setFont(fBold); // default action with double click
    ac->setData("details");
    menu->addAction(ac);

    ac = new QAction(this);
    ac->setSeparator(true);
    menu->addAction(ac);

    ac = new QAction(this);
    ac->setText(tr("Lookup in tree"));
    ac->setData("lookup");
    menu->addAction(ac);

    ac = new QAction(this);
    ac->setText(tr("Copy node path"));
    ac->setData("copy");
    menu->addAction(ac);

    // Show the context menu and check selected action
    ac = menu->exec(globalPos);
    if (ac && ac->isEnabled() && !ac->isSeparator()) {
        if (ac->data().toString() == "details") {
            showDetails(indexClicked);
        }
        else if (ac->data().toString() == "lookup") {
            lookup(indexClicked);
        }
        else if (ac->data().toString() == "copy") {
            copyPath(indexClicked);
        }
    }

    delete menu;
}

void TimelineView::showDetails(const QModelIndex& indexClicked) {
    QModelIndex idx = model_->mapToSource(indexClicked);
    if (!idx.isValid()) {
        return;
    }

    if (!infoDialog_) {
        infoDialog_ = new TimelineInfoDialog(this);
        infoDialog_->setModal(false);
    }

    if (idx.row() == infoDialog_->infoW_->itemIndex()) {
        infoDialog_->raise();
    }
    else {
        infoDialog_->infoW_->load("host", "port", model_->tlModel()->data(), idx.row(), startDate_, endDate_);
    }

    infoDialog_->show();
}

void TimelineView::updateDetails() {
    if (infoDialog_ && infoDialog_->isVisible() && infoDialog_->infoW_->itemIndex() != -1) {
        infoDialog_->infoW_->load(
            "host", "port", model_->tlModel()->data(), infoDialog_->infoW_->itemIndex(), startDate_, endDate_);
    }
}

void TimelineView::closeDetails() {
    if (infoDialog_) {
        infoDialog_->close();
    }
}

void TimelineView::dataCleared() {
    closeDetails();
}

void TimelineView::lookup(const QModelIndex& indexClicked) {
    QModelIndex idx = model_->mapToSource(indexClicked);
    if (!idx.isValid()) {
        return;
    }

    QString nodePath = QString::fromStdString(model_->tlModel()->data()->items()[idx.row()].path());
    Q_EMIT(lookupRequested(nodePath));
}

void TimelineView::copyPath(const QModelIndex& indexClicked) {
    QModelIndex idx = model_->mapToSource(indexClicked);
    if (!idx.isValid()) {
        return;
    }

    QString nodePath = QString::fromStdString(model_->tlModel()->data()->items()[idx.row()].path());
    Q_EMIT(copyPathRequested(nodePath));
}

void TimelineView::slotHzScrollbar(int, int) {
    if (QScrollBar* sb = horizontalScrollBar()) {
        sb->setValue(sb->maximum());
    }
}

void TimelineView::slotViewCommand(VInfo_ptr /*info*/, QString /*cmd*/) {
}

void TimelineView::rerender() {
    if (needItemsLayout_) {
        doItemsLayout();
        needItemsLayout_ = false;
    }
    else {
        viewport()->update();
    }
}

void TimelineView::slotRerender() {
    rerender();
}

void TimelineView::slotSizeHintChangedGlobal() {
    needItemsLayout_ = true;
}

void TimelineView::adjustBackground(QColor col) {
    if (col.isValid()) {
        QString sh = "QTreeView { background : " + col.name() + ";}";
        setStyleSheet(sh);
    }
}

void TimelineView::notifyChange(VProperty* p) {
    if (p->path() == "view.table.background") {
        adjustBackground(p->value().value<QColor>());
    }
}

void TimelineView::setZoomActions(QAction* zoomInAction, QAction* zoomOutAction) {
    header_->setZoomActions(zoomInAction, zoomOutAction);
}

void TimelineView::periodSelectedInHeader(QDateTime t1, QDateTime t2) {
    startDate_ = t1;
    endDate_   = t2;
    delegate_->setPeriod(t1, t2);
    model_->tlModel()->setPeriod(t1, t2);
    updateDetails();
    updateDurations();
    rerender();
    Q_EMIT periodSelected(t1, t2);
}

void TimelineView::setStartDate(QDateTime t) {
    startDate_ = t;
    delegate_->setStartDate(t);
    model_->tlModel()->setStartDate(t);
    header_->setStartDate(t);
    updateDetails();
    updateDurations();
    rerender();
}

void TimelineView::setEndDate(QDateTime t) {
    endDate_ = t;
    delegate_->setEndDate(t);
    model_->tlModel()->setEndDate(t);
    header_->setEndDate(t);
    updateDetails();
    updateDurations();
    rerender();
}

void TimelineView::setPeriod(QDateTime t1, QDateTime t2) {
    startDate_ = t1;
    endDate_   = t2;
    delegate_->setPeriod(t1, t2);
    model_->tlModel()->setPeriod(t1, t2);
    header_->setPeriod(t1, t2);
    updateDetails();
    updateDurations();
    rerender();
}

void TimelineView::setDurationViewMode(DurationViewMode mode) {
    durationViewMode_ = mode;
    delegate_->setUseMeanDuration(durationViewMode_ == MeanDurationMode);
    model_->tlModel()->setUseMeanDuration(durationViewMode_ == MeanDurationMode);
    if (viewMode_ == DurationMode) {
        updateDurations();
        model_->sortAgain();
        // rerender();
    }
}

void TimelineView::updateDurations() {
    if (viewMode_ == DurationMode) {
        int submittedDuration = computeMaxDuration("submitted");
        int activeDuration    = computeMaxDuration("active");
        delegate_->setMaxDurations(submittedDuration, activeDuration);
        header_->setMaxDurations(submittedDuration, activeDuration);
    }
}

void TimelineView::setViewMode(ViewMode vm, bool force) {
    if (vm != viewMode_ || force) {
        viewMode_ = vm;
        adjustHeader();
        header_->viewModeChanged();

        if (viewMode_ == DurationMode) {
            updateDurations();
            if (!durationColumnWidthInitialised_) {
                durationColumnWidthInitialised_ = true;
                int w1                          = columnWidth(TimelineModel::SubmittedDurationColumn);
                int w2                          = columnWidth(TimelineModel::ActiveDurationColumn);
                w1                              = (w2 + w1) / 2;
                if (w1 > 10) {
                    setColumnWidth(TimelineModel::SubmittedDurationColumn, w1);
                }
            }
            setSortingEnabled(true);
            sortByColumn(TimelineModel::ActiveDurationColumn, Qt::DescendingOrder);
        }
        else {
            setSortingEnabled(false);
        }

        // header_->viewModeChanged();
        rerender();
    }
}

void TimelineView::adjustHeader() {
    if (headerBeingAdjusted_) {
        return;
    }

    headerBeingAdjusted_ = true;

    if (viewMode_ == TimelineMode) {
        header_->setSectionHidden(TimelineModel::SubmittedDurationColumn, true);
        header_->setSectionHidden(TimelineModel::ActiveDurationColumn, true);
        header_->setSectionHidden(TimelineModel::TimelineColumn, false);
    }
    else if (viewMode_ == DurationMode) {
        header_->setSectionHidden(TimelineModel::SubmittedDurationColumn, false);
        header_->setSectionHidden(TimelineModel::ActiveDurationColumn, false);
        header_->setSectionHidden(TimelineModel::TimelineColumn, true);
    }

    headerBeingAdjusted_ = false;
}

int TimelineView::computeMaxDuration(QString state) {
    int maxDuration = -1;
    int col         = 0;

    if (state == "submitted") {
        col = TimelineModel::SubmittedDurationColumn;
    }
    else if (state == "active") {
        col = TimelineModel::ActiveDurationColumn;
    }
    else {
        return -1;
    }

    for (int i = 0; i < model_->rowCount(); i++) {
        int val         = 0;
        QModelIndex idx = model_->index(i, col);
        if (durationViewMode_ == FirstDurationMode) {
            val = model_->data(idx).toInt();
        }
        else {
            QVariantList lst = model_->data(idx, TimelineModel::MeanDurationRole).toList();
            if (lst.count() == 2) {
                val = lst[0].toFloat();
            }
        }

        if (val > maxDuration) {
            maxDuration = val;
        }
    }
    return maxDuration;
}

//=========================================
// Header
//=========================================

void TimelineView::slotHeaderContextMenu(const QPoint& /*position*/) {
}

void TimelineView::readSettings(VSettings* vs) {
    vs->beginGroup("view");
    std::vector<int> wVec;
    vs->get("width", wVec);
    vs->endGroup();

    for (size_t i = 0; i < wVec.size() && i < static_cast<size_t>(model_->columnCount()); i++) {
        if (wVec[i] > 0) {
            setColumnWidth(i, wVec[i]);
        }
    }
}

void TimelineView::writeSettings(VSettings* vs) {
    vs->beginGroup("view");

    std::vector<int> wVec;
    for (int i = 0; i < model_->columnCount(QModelIndex()); i++) {
        wVec.push_back(columnWidth(i));
    }

    vs->put("width", wVec);

    vs->endGroup();
}
