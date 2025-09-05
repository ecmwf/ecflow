/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TimelineHeaderView.hpp"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QTreeView>
#include <QtGlobal>

#include "IconProvider.hpp"
#include "TimelineModel.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"
#include "ViewerUtil.hpp"

#if 0
MainTimelineHeader::MainTimelineHeader(QTreeView *view) : TimelineHeader(view)
{
    columnType_ << OtherColumn;
    columnType_ << TimelineColumn;
    columnType_ << OtherColumn;
    columnType_ << OtherColumn;
}

NodeTimelineHeader::NodeTimelineHeader(QTreeView *view) : TimelineHeader(view)
{
    columnType_ << OtherColumn;
    columnType_ << DayColumn;
}

#endif

TimelineHeader::TimelineHeader(QTreeView* view)
    : QHeaderView(Qt::Horizontal, view),
      view_(view),
      fm_(QFont()),
      timelineCol_(50, 50, 50),
      dateTextCol_(33, 95, 161),
      timeTextCol_(30, 30, 30),
      timelineFrameBorderCol_(150, 150, 150),
      timelineFrameSize_(4),
      majorTickSize_(5),
      zoomCol_(224, 236, 248, 190),
      inZoom_(false),
      zoomInAction_(nullptr),
      zoomOutAction_(nullptr)
// submittedMaxDuration_(-1),
// activeMaxDuration_(-1)
{
    Q_ASSERT(view_);

    setMouseTracking(true);

    setStretchLastSection(true);

    QColor bg0(214, 214, 214);
    QColor bg1(194, 194, 194);
    QLinearGradient gr;
    gr.setCoordinateMode(QGradient::ObjectBoundingMode);
    gr.setStart(0, 0);
    gr.setFinalStop(0, 1);
    gr.setColorAt(0, bg0);
    gr.setColorAt(1, bg1);
    timelineBrush_ = QBrush(gr);

    font_ = QFont();
    font_.setPointSize(font_.pointSize() - 2);
    fm_ = QFontMetrics(font_);

    zoomCursor_ = QCursor(QPixmap(":/viewer/cursor_zoom.svg"));
}

QSize TimelineHeader::sizeHint() const {
    QSize s = QHeaderView::sizeHint(); // size();
    if (hasTimeColumn()) {
        s.setHeight(timelineFrameSize_ + fm_.height() + 6 + majorTickSize_ + fm_.height() + 6 + timelineFrameSize_);
    }
    return s;
}

QPoint TimelineHeader::realPos(QPoint pos) const {
    QPoint scrollOffset(view_->horizontalScrollBar()->value(), view_->verticalScrollBar()->value());
    return pos + scrollOffset;
}

// Event pos is based on the visible portion of the widget! this is not the
// real postion when scrollbars are visible!
void TimelineHeader::mousePressEvent(QMouseEvent* event) {
    // Start new zoom
    if (isZoomEnabled() && !inZoom_ && event->button() == Qt::LeftButton && isColumnZoomable(event->pos()) &&
        canBeZoomed()) {
        zoomStartPos_ = event->pos();
    }
    else {
        QHeaderView::mousePressEvent(event);
    }
}

void TimelineHeader::mouseMoveEvent(QMouseEvent* event) {
    // When we enter the timeline section we show a zoom cursor
    bool hasCursor = testAttribute(Qt::WA_SetCursor);

    if (!isZoomEnabled()) {
        QHeaderView::mouseMoveEvent(event);
        return;
    }

    // When enabled ...

    if (event->buttons().testFlag(Qt::LeftButton)) {
        int columnIndex = logicalIndexAt(zoomStartPos_); // logicalIndexAt takes "visible position" -- scrollbars
                                                         // ignored
        int secStart = sectionPosition(columnIndex);     // real position - scroolbars taken into account
        int secEnd   = secStart + sectionSize(columnIndex);

        // If we are in resize mode
        if (!inZoom_ && zoomStartPos_.isNull()) {
            QHeaderView::mouseMoveEvent(event);
            return;
        }

        // In timeline zoom mode we show a zoom cursor
        if (!hasCursor || cursor().shape() == Qt::SplitHCursor) {
            setCursor(zoomCursor_);
        }

        inZoom_     = true;
        zoomEndPos_ = event->pos();

        QPoint rPos = realPos(event->pos());
        int deltaX  = rPos.x() - event->pos().x();

        if (rPos.x() >= secStart && rPos.x() <= secEnd) {
            if (event->pos().x() < zoomStartPos_.x()) {
                zoomEndPos_ = zoomStartPos_;
            }
        }
        else if (rPos.x() < secStart) {
            zoomEndPos_ = zoomStartPos_;
        }
        else // if(event->pos().x() > secEnd)
        {
            zoomEndPos_ = event->pos();
            zoomEndPos_.setX(secEnd - deltaX - 1);
        }

        // we need to specify the full range here because otherwise the
        // beginning of the timeline section is clipped
        headerDataChanged(Qt::Horizontal, 0, columnIndex);

        if (isColumnZoomableAtIndex(columnIndex)) {
            beingZoomedCore();
        }
    }
    else {
        // When we enter the timeline section we show a zoom cursor
        if (isColumnZoomable(event->pos())) {
            if ((!hasCursor || cursor().shape() == Qt::SplitHCursor) && (canBeZoomed())) {
                setCursor(zoomCursor_);
            }
        }
        // Otherwise remove the cursor unless it is the resize indicator
        else {
            if (hasCursor && cursor().shape() != Qt::SplitHCursor) {
                unsetCursor();
            }

            QHeaderView::mouseMoveEvent(event);
        }
    }
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent* event) {
    if (inZoom_) {
        int columnIndex = logicalIndexAt(zoomStartPos_);
        if (isColumnZoomableAtIndex(columnIndex)) {
            doZoom();
        }
    }
    else {
        QHeaderView::mouseReleaseEvent(event);
    }
}

void TimelineHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
    painter->save();

    if (!rect.isValid()) {
        return;
    }

    QStyleOptionHeader opt;
    initStyleOption(&opt);
    QStyle::State state = QStyle::State_None;
    if (isEnabled()) {
        state |= QStyle::State_Enabled;
    }
    if (window()->isActiveWindow()) {
        state |= QStyle::State_Active;
    }

    // if(isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
    //        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
    //                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    // setup the style options structure
    // QVariant textAlignment = model->headerData(logicalIndex, d->orientation,
    //                                                 Qt::TextAlignmentRole);
    opt.rect    = rect;
    opt.section = logicalIndex;
    opt.state |= state;
    // opt.textAlignment = Qt::Alignment(textAlignment.isValid()
    //                                      ? Qt::Alignment(textAlignment.toInt())
    //                                      : d->defaultAlignment);

    // opt.text = model()->headerData(logicalIndex, Qt::Horizontal),
    //                                     Qt::DisplayRole).toString();

    QVariant foregroundBrush;
    if (foregroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));
    }

    QPointF oldBO = painter->brushOrigin();
    QVariant backgroundBrush;
    if (backgroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
        painter->setBrushOrigin(opt.rect.topLeft());
    }

    // the section position
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);

    if (count() == 1) {
        opt.position = QStyleOptionHeader::OnlyOneSection;
    }
    else if (visual == 0) {
        opt.position = QStyleOptionHeader::Beginning;
    }
    else if (visual == count() - 1) {
        opt.position = QStyleOptionHeader::End;
    }
    else {
        opt.position = QStyleOptionHeader::Middle;
    }

    opt.orientation = Qt::Horizontal;

    // the selected position
    /*bool previousSelected = d->isSectionSelected(logicalIndex(visual - 1));
        bool nextSelected =  d->isSectionSelected(logicalIndex(visual + 1));
        if (previousSelected && nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
        else if (previousSelected)
            opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
        else if (nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
        else
            opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    */

    // draw the section
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->setBrushOrigin(oldBO);

    painter->restore();

    int rightPos = rect.right();
    if (view_->isSortingEnabled()) {
        if (isSortIndicatorShown() && sortIndicatorSection() == logicalIndex) {
            opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder) ? QStyleOptionHeader::SortDown
                                                                             : QStyleOptionHeader::SortUp;
        }

        if (opt.sortIndicator != QStyleOptionHeader::None) {
            QStyleOptionHeader subopt = opt;
            subopt.rect               = style()->subElementRect(QStyle::SE_HeaderArrow, &opt, this);
            rightPos                  = subopt.rect.left();
            style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &subopt, painter, this);
        }
    }

    if (isTimelineColumn(logicalIndex)) {
        renderTimeline(rect, painter, logicalIndex);
    }
    else {
        QString text   = model()->headerData(logicalIndex, Qt::Horizontal).toString();
        QRect textRect = rect;
        textRect.setRight(rightPos - 5);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, " " + text);
    }

    painter->restore();
}

void TimelineHeader::setZoomActions(QAction* zoomInAction, QAction* zoomOutAction) {
    if (!zoomInAction_) {
        zoomInAction_  = zoomInAction;
        zoomOutAction_ = zoomOutAction;

        connect(zoomInAction_, SIGNAL(toggled(bool)), this, SLOT(slotZoomState(bool)));

        connect(zoomOutAction_, SIGNAL(triggered(bool)), this, SLOT(slotZoomOut(bool)));

        checkActionState();
    }
}

void TimelineHeader::enableZoomActions(bool b) {
    if (zoomInAction_) {
        Q_ASSERT(zoomOutAction_);
        zoomInAction_->setEnabled(b);
        zoomOutAction_->setEnabled(b);
    }
}

bool TimelineHeader::isZoomEnabled() const {
    return (zoomInAction_ && zoomInAction_->isEnabled()) ? (zoomInAction_->isChecked()) : false;
}

void TimelineHeader::setZoomDisabled() {
    if (zoomInAction_) {
        zoomInAction_->setChecked(false);
    }
}

void TimelineHeader::slotZoomState(bool) {
    Q_ASSERT(zoomInAction_);
    if (!zoomInAction_->isChecked()) {
        bool hasCursor = testAttribute(Qt::WA_SetCursor);
        if (hasCursor && cursor().shape() != Qt::SplitHCursor) {
            unsetCursor();
        }
    }

    // headerDataChanged(Qt::Horizontal,0,TimelineModel::TimelineColumn);
}

bool TimelineHeader::isColumnZoomable(QPoint pos) const {
    int logicalIndex = logicalIndexAt(pos);
    if (logicalIndex != -1) {
        return isColumnZoomableAtIndex(logicalIndex);
    }

    return false;
}

bool TimelineHeader::hasZoomableColumn() const {
    for (int i = 0; i < columnType_.count(); i++) {
        if (!isSectionHidden(i) && isColumnZoomableAtIndex(i)) {
            return true;
        }
    }
    return false;
}

bool TimelineHeader::hasTimeColumn() const {
    for (int i = 0; i < columnType_.count(); i++) {
        if (!isSectionHidden(i) && isColumnZoomableAtIndex(i)) {
            return true;
        }
    }
    return false;
}

void TimelineHeader::checkActionState() {
    if (zoomInAction_) {
        zoomOutAction_->setEnabled(zoomHistoryCount() >= 2);
        zoomInAction_->setEnabled(canBeZoomed());
    }
}

int TimelineHeader::secToPosPeriod(qint64 t, QRect rect, qint64 period) const {
    return rect.x() +
           static_cast<int>(static_cast<float>(t) / static_cast<float>(period) * static_cast<float>(rect.width()));
}

void TimelineHeader::viewModeChanged() {
    if (hasZoomableColumn()) {
        enableZoomActions(true);
    }
    else {
        enableZoomActions(false);
    }
    checkActionState();

#if 0
    if(!hasZoomableColumn()) //????
    {
        zoomHistory_.clear();
    }
    checkActionState();
#endif
}

void TimelineHeader::rerender() {
    headerDataChanged(Qt::Horizontal, 0, count() - 1);
}

//=================================================================
//
// MainTimelineHeader
//
//=================================================================

MainTimelineHeader::MainTimelineHeader(QTreeView* view)
    : TimelineHeader(view),
      submittedMaxDuration_(-1),
      activeMaxDuration_(-1) {
    Q_ASSERT(view_);
    columnType_ << OtherColumn;
    columnType_ << TimelineColumn;
    columnType_ << OtherColumn;
    columnType_ << OtherColumn;
}

void MainTimelineHeader::doZoom() {
    QDateTime sDt = posToDate(zoomStartPos_);
    QDateTime eDt = posToDate(zoomEndPos_);
    zoomStartPos_ = QPoint();
    zoomEndPos_   = QPoint();
    inZoom_       = false;
    if (sDt.isValid() && eDt.isValid()) {
        setPeriodCore(sDt, eDt, true);
        Q_EMIT periodSelected(sDt, eDt);
    }
    else {
        rerender();
        Q_EMIT periodBeingZoomed(startDate_, endDate_);
    }

    setZoomDisabled();
    checkActionState();
}

void MainTimelineHeader::beingZoomedCore() {
    QDateTime sDt = posToDate(zoomStartPos_);
    QDateTime eDt = posToDate(zoomEndPos_);
    if (sDt.isValid() && eDt.isValid()) {
        Q_EMIT periodBeingZoomed(sDt, eDt);
    }
}

bool MainTimelineHeader::canBeZoomed() const {
    if (hasZoomableColumn()) {
        return (endDate_.toMSecsSinceEpoch() - startDate_.toMSecsSinceEpoch()) > 60 * 1000;
    }
    return false;
}

void MainTimelineHeader::renderTimeline(const QRect& rect, QPainter* painter, int logicalIndex) const {
    painter->save();

    // painter->fillRect(rect.adjusted(0,0,0,-1),timelineFrameBgCol_);

    // The timeline area bounded by the frame
    QRect pRect = rect.adjusted(0, timelineFrameSize_, 0, -timelineFrameSize_);

    painter->setClipRect(rect);

    // Special appearance for the timeline area
    painter->fillRect(pRect, timelineBrush_);

    painter->setPen(QPen(timelineFrameBorderCol_));
    painter->drawLine(QPoint(rect.left(), pRect.top()), QPoint(rect.right(), pRect.top()));

    painter->drawLine(QPoint(rect.left(), pRect.bottom()), QPoint(rect.right(), pRect.bottom()));

    if (inZoom_) {
        int sStart    = sectionPosition(logicalIndex);
        QPoint zStart = realPos(zoomStartPos_);
        QPoint zEnd   = realPos(zoomEndPos_);
        zStart += QPoint(rect.x() - sStart, 0);
        zEnd += QPoint(rect.x() - sStart, 0);

        QRect zRect = pRect;
        zRect.setLeft(zStart.x());
        zRect.setRight(zEnd.x());
        painter->fillRect(zRect, zoomCol_);
        painter->setPen(zoomCol_.darker(140));
        painter->drawRect(zRect.adjusted(0, 1, 0, -2));
    }

    int w = sectionSize(logicalIndex);

    // period in secs
    qint64 startSec = startDate_.toMSecsSinceEpoch() / 1000;
    qint64 endSec   = endDate_.toMSecsSinceEpoch() / 1000;
    qint64 period   = endSec - startSec;

    int minorTick    = 1; // in secs (it is a delta)
    int majorTick    = 1; // in secs (it is a delta)
    qint64 firstTick = 1; // in secs since epoch

    int hLineY = pRect.center().y() - majorTickSize_ / 2 - 1;
    // int timeTextGap=3; //the gap between the top of the time text and the bottom of the major tick in pixels
    int majorTickTop    = hLineY;
    int majorTickBottom = hLineY + majorTickSize_; // pRect.bottom()-fm_.height()-timeTextGap;
    int minorTickTop    = hLineY;
    int minorTickBottom = majorTickBottom - 3;
    int dateTextY       = hLineY - (hLineY - pRect.y() - fm_.height()) / 2 - fm_.height() + 1;
    int timeTextY       = majorTickBottom + (pRect.bottom() - majorTickBottom - fm_.height()) / 2 - 1;

    int timeItemW = ViewerUtil::textWidth(fm_, "223:442");
    int dateItemW = ViewerUtil::textWidth(fm_, "2229 May22");

    QList<int> majorTickSec;

    if (period < 60) {
        majorTickSec << 1 << 5 << 10 << 20 << 30;
    }
    else if (period < 600) {
        majorTickSec << 10 << 15 << 20 << 30 << 60 << 5 * 60 << 10 * 60;
    }
    if (period < 3600) {
        majorTickSec << 60 << 2 * 60 << 3 * 60 << 4 * 60 << 5 * 60 << 10 * 60 << 20 * 60 << 30 * 60;
    }
    if (period < 12 * 3600) {
        majorTickSec << 15 * 60 << 30 * 60 << 3600 << 2 * 3600 << 3 * 3600 << 4 * 3600 << 6 * 3600;
    }
    else if (period <= 86400) {
        majorTickSec << 3600 << 2 * 3600 << 3 * 3600 << 4 * 3600 << 6 * 3600 << 12 * 3600;
    }
    else if (period < 7 * 86400) {
        majorTickSec << 3600 << 2 * 3600 << 3 * 3600 << 4 * 3600 << 6 * 3600 << 12 * 3600 << 24 * 3600;
    }
    else if (period < 14 * 86400) {
        majorTickSec << 12 * 3600 << 24 * 3600 << 36 * 3600 << 48 * 3600 << 72 * 3600 << 96 * 3600;
    }
    else if (period < 28 * 86400) {
        majorTickSec << 86400 << 2 * 86400 << 3 * 86400 << 4 * 86400 << 5 * 86400 << 10 * 86400;
    }
    else if (period < 60 * 86400) {
        majorTickSec << 86400 << 2 * 86400 << 3 * 86400 << 4 * 86400 << 5 * 86400 << 10 * 86400 << 20 * 86400;
    }
    else if (period < 365 * 86400) {
        majorTickSec << 5 * 86400 << 10 * 86400 << 20 * 86400 << 30 * 86400 << 60 * 86400 << 90 * 86400 << 180 * 86400;
    }
    else {
        majorTickSec << 30 * 86400 << 60 * 86400 << 90 * 86400 << 180 * 86400 << 365 * 86400;
    }

    Q_FOREACH (int mts, majorTickSec) {
        majorTick        = mts;
        int majorTickNum = period / majorTick;
        int cover        = timeItemW * majorTickNum;
        int diff         = w - cover;
        if (diff > 100) {
            break;
        }
    }

    minorTick = majorTick / 4;
    if (minorTick == 0) {
        minorTick = majorTick;
    }

    firstTick = (startSec / minorTick) * minorTick + minorTick;

    // Find label positions for days
    QList<QPair<int, QString>> dateLabels;
    int dayNum = startDate_.date().daysTo(endDate_.date());

    if (dayNum == 0) {
        int xp = secToPos((startSec + endSec) / 2 - startSec, rect);
        dateLabels << qMakePair(xp, startDate_.toString("dd MMM"));
    }
    else {
        QDate nextDay  = startDate_.date().addDays(1);
        QDate lastDay  = endDate_.date();
        qint64 nextSec = QDateTime(nextDay, QTime()).toMSecsSinceEpoch() / 1000;

        if ((nextSec - startSec) < 3600) {
            int xp = secToPos((startSec + nextSec) / 2 - startSec, rect);
            dateLabels << qMakePair(xp, nextDay.toString("dd MMM"));
        }

        int dayFreq = 1;
        QList<int> dayFreqLst;
        dayFreqLst << 1 << 2 << 3 << 5 << 10 << 20 << 30 << 60 << 90 << 120 << 180 << 365;
        Q_FOREACH (int dfv, dayFreqLst) {
            if ((dayNum / dfv) * dateItemW < w - 100) {
                dayFreq = dfv;
                break;
            }
        }

        QDate firstDay = nextDay;
        for (QDate d = firstDay; d < lastDay; d = d.addDays(dayFreq)) {
            int xp = secToPos((QDateTime(d, QTime()).toMSecsSinceEpoch() / 1000 +
                               QDateTime(d.addDays(1), QTime()).toMSecsSinceEpoch() / 1000) /
                                      2 -
                                  startSec,
                              rect);
            dateLabels << qMakePair(xp, d.toString("dd MMM"));
        }

        if (QDateTime(lastDay, QTime()).toMSecsSinceEpoch() / 1000 < endSec) {
            int xp = secToPos((QDateTime(lastDay, QTime()).toMSecsSinceEpoch() / 1000 + endSec) / 2 - startSec, rect);
            dateLabels << qMakePair(xp, lastDay.toString("dd MMM"));
        }
    }

    // Draw date labels
    painter->setPen(dateTextCol_);
    for (int i = 0; i < dateLabels.count(); i++) {
        int xp    = dateLabels[i].first;
        int textW = ViewerUtil::textWidth(fm_, dateLabels[i].second);
        // int yp=rect.bottom()-1-fm.height();
        painter->setFont(font_);
        painter->drawText(QRect(xp - textW / 2, dateTextY, textW, fm_.height()),
                          Qt::AlignHCenter | Qt::AlignVCenter,
                          dateLabels[i].second);
    }

    // horizontal line
    painter->setPen(timelineCol_);
    painter->drawLine(rect.x(), hLineY, rect.right(), hLineY);

    qint64 actSec = firstTick;
    Q_ASSERT(actSec >= startSec);
    painter->setPen(timeTextCol_);

    while (actSec <= endSec) {
        int xp = secToPos(actSec - startSec, rect);

        // draw major tick + label
        if (actSec % majorTick == 0) {
            painter->drawLine(xp, majorTickTop, xp, majorTickBottom);

            QString s;
            if (majorTick < 60) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000, Qt::UTC).toString("H:mm:ss");
#else
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000).toUTC().toString("H:mm:ss");
#endif
            }
            else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000, Qt::UTC).toString("H:mm");
#else
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000).toUTC().toString("H:mm");
#endif
            }

            int textW = ViewerUtil::textWidth(fm_, s);
            painter->setFont(font_);
            painter->drawText(
                QRect(xp - textW / 2, timeTextY, textW, fm_.height()), Qt::AlignHCenter | Qt::AlignVCenter, s);
        }
        // draw minor tick
        else {
            painter->drawLine(xp, minorTickTop, xp, minorTickBottom);
        }

        actSec += minorTick;
    }

    painter->restore();
}

int MainTimelineHeader::secToPos(qint64 t, QRect rect) const {
    // qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period = (endDate_.toMSecsSinceEpoch() - startDate_.toMSecsSinceEpoch()) / 1000;
    return rect.x() +
           static_cast<int>(static_cast<float>(t) / static_cast<float>(period) * static_cast<float>(rect.width()));
}

// pos is based on the visible portion of the widget - scrollbars are ignored
QDateTime MainTimelineHeader::posToDate(QPoint pos) const {
    int logicalIndex = logicalIndexAt(pos);
    if (logicalIndex == -1) {
        return {};
    }

    int xp = sectionPosition(logicalIndex);
    int w  = sectionSize(logicalIndex);

    // take the scrollbar into account
    QPoint rPos = realPos(pos);

    if (w <= 0 || rPos.x() < xp) {
        return {};
    }

    double r = static_cast<double>(rPos.x() - xp) / static_cast<double>(w);
    if (r < 0 || r > 1) {
        return {};
    }

    // qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period = (endDate_.toMSecsSinceEpoch() - startDate_.toMSecsSinceEpoch()) / 1000;

    return startDate_.addMSecs((r * static_cast<double>(period)) * 1000);
}

void MainTimelineHeader::slotZoomOut(bool) {
    if (!inZoom_ && !isZoomEnabled()) {
        if (zoomHistory_.count() >= 2) {
            zoomHistory_.pop();
            QDateTime sDt = zoomHistory_.top().first;
            QDateTime eDt = zoomHistory_.top().second;
            if (sDt.isValid() && eDt.isValid()) {
                setPeriodCore(sDt, eDt, false);
                Q_EMIT periodSelected(sDt, eDt);
            }

            checkActionState();
        }
    }
}

void MainTimelineHeader::setStartDate(QDateTime t) {
    startDate_ = t;
    zoomHistory_.clear();
    zoomHistory_.push(QPair<QDateTime, QDateTime>(startDate_, endDate_));
    checkActionState();
    rerender();
}

void MainTimelineHeader::setEndDate(QDateTime t) {
    endDate_ = t;
    zoomHistory_.clear();
    zoomHistory_.push(QPair<QDateTime, QDateTime>(startDate_, endDate_));
    checkActionState();
    rerender();
}

void MainTimelineHeader::setPeriod(QDateTime t1, QDateTime t2) {
    zoomHistory_.clear();
    setPeriodCore(t1, t2, true);
}

void MainTimelineHeader::setPeriodCore(QDateTime t1, QDateTime t2, bool addToHistory) {
    startDate_ = t1;
    endDate_   = t2;
    if (addToHistory) {
        zoomHistory_.push(QPair<QDateTime, QDateTime>(startDate_, endDate_));
    }
    checkActionState();
    rerender();
}

bool MainTimelineHeader::isColumnZoomableAtIndex(int logicalIndex) const {
    return columnType_[logicalIndex] == TimelineColumn;
}

bool MainTimelineHeader::isTimelineColumn(int logicalIndex) const {
    return columnType_[logicalIndex] == TimelineColumn;
}

void MainTimelineHeader::setMaxDurations(int submittedDuration, int activeDuration) {
    submittedMaxDuration_ = submittedDuration;
    activeMaxDuration_    = activeDuration;
}

//=================================================================
//
// NodeTimelineHeader
//
//=================================================================

NodeTimelineHeader::NodeTimelineHeader(QTreeView* view) : TimelineHeader(view) {
    columnType_ << OtherColumn;
    columnType_ << DayColumn;
}

void NodeTimelineHeader::doZoom() {
    QTime st      = posToTime(zoomStartPos_);
    QTime et      = posToTime(zoomEndPos_);
    zoomStartPos_ = QPoint();
    zoomEndPos_   = QPoint();
    inZoom_       = false;
    if (st.isValid() && et.isValid()) {
        setPeriodCore(st, et, true);
        Q_EMIT periodSelected(st, et);
    }
    else {
        rerender();
        Q_EMIT periodBeingZoomed(startTime_, endTime_);
    }

    setZoomDisabled();
    checkActionState();
}

void NodeTimelineHeader::slotZoomOut(bool) {
    if (!inZoom_ && !isZoomEnabled()) {
        if (zoomHistory_.count() >= 2) {
            zoomHistory_.pop();
            QTime st = zoomHistory_.top().first;
            QTime et = zoomHistory_.top().second;
            if (st.isValid() && et.isValid()) {
                setPeriodCore(st, et, false);
                Q_EMIT periodSelected(st, et);
            }
            checkActionState();
        }
    }
}

void NodeTimelineHeader::beingZoomedCore() {
    QTime st = posToTime(zoomStartPos_);
    QTime et = posToTime(zoomEndPos_);
    if (st.isValid() && et.isValid()) {
        Q_EMIT periodBeingZoomed(st, et);
    }
}

void NodeTimelineHeader::renderTimeline(const QRect& rect, QPainter* painter, int logicalIndex) const {
    painter->save();

    // painter->fillRect(rect.adjusted(0,0,0,-1),timelineFrameBgCol_);

    // The timeline area bounded by the frame
    QRect pRect = rect.adjusted(0, timelineFrameSize_, 0, -timelineFrameSize_);

    // Special appearance for the timeline area
    painter->fillRect(pRect, timelineBrush_);

    painter->setPen(QPen(timelineFrameBorderCol_));
    painter->drawLine(QPoint(rect.left(), pRect.top()), QPoint(rect.right(), pRect.top()));

    painter->drawLine(QPoint(rect.left(), pRect.bottom()), QPoint(rect.right(), pRect.bottom()));

    if (inZoom_) {
        int sStart    = sectionPosition(logicalIndex);
        QPoint zStart = realPos(zoomStartPos_);
        QPoint zEnd   = realPos(zoomEndPos_);
        zStart += QPoint(rect.x() - sStart, 0);
        zEnd += QPoint(rect.x() - sStart, 0);

        QRect zRect = pRect;
        zRect.setLeft(zStart.x());
        zRect.setRight(zEnd.x());
        painter->fillRect(zRect, zoomCol_);
        painter->setPen(zoomCol_.darker(140));
        painter->drawRect(zRect.adjusted(0, 1, 0, -2));
    }

    int w = sectionSize(logicalIndex);

    // period in secs
    qint64 startSec = startTime_.msecsSinceStartOfDay() / 1000;
    qint64 endSec   = endTime_.msecsSinceStartOfDay() / 1000;
    qint64 period   = endSec - startSec;

    int minorTick    = 1; // in secs (it is a delta)
    int majorTick    = 1; // in secs (it is a delta)
    qint64 firstTick = 1; // in secs since epoch

    int hLineY          = pRect.center().y() - majorTickSize_ / 2 - 1;
    int majorTickTop    = hLineY;
    int majorTickBottom = hLineY + majorTickSize_; // pRect.bottom()-fm_.height()-timeTextGap;
    int minorTickTop    = hLineY;
    int minorTickBottom = majorTickBottom - 3;
    int timeTextY       = majorTickBottom + (pRect.bottom() - majorTickBottom - fm_.height()) / 2 - 1;

    int timeItemW = ViewerUtil::textWidth(fm_, "223:442");

    QList<int> majorTickSec;

    if (period < 600) {
        majorTickSec << 60;
    }
    if (period < 1800) {
        majorTickSec << 5 * 60;
    }
    else if (period < 7200) {
        majorTickSec << 10 * 60 << 15 * 60 << 30 * 60;
    }
    if (period < 12 * 3600) {
        majorTickSec << 15 * 60 << 30 * 60 << 60 * 60 << 120 * 60 << 180 * 60;
    }
    else {
        majorTickSec << 30 * 60 << 3600 << 2 * 3600 << 3 * 3600 << 4 * 3600;
    }

    Q_FOREACH (int mts, majorTickSec) {
        majorTick        = mts;
        int majorTickNum = period / majorTick;
        int cover        = timeItemW * majorTickNum;
        int diff         = w - cover;
        if (diff > 100) {
            break;
        }
    }

    minorTick = majorTick / 4;
    if (minorTick == 0) {
        minorTick = majorTick;
    }

    firstTick = (startSec / minorTick) * minorTick;
    if (firstTick < startSec) {
        firstTick += minorTick;
    }

    // Find label positions for days
    QList<QPair<int, QString>> dateLabels;

    painter->save();
    painter->setClipRect(rect);

    // horizontal line
    painter->setPen(timelineCol_);
    painter->drawLine(rect.x(), hLineY, rect.right(), hLineY);

    qint64 actSec = firstTick;
    Q_ASSERT(actSec >= startSec);
    painter->setPen(timeTextCol_);

    while (actSec <= endSec) {
        int xp = secToPosPeriod(actSec - startSec, rect, period);

        // draw major tick + label
        if (actSec % majorTick == 0) {
            painter->drawLine(xp, majorTickTop, xp, majorTickBottom);

            QString s;
            if (majorTick < 60) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000, Qt::UTC).toString("H:mm:ss");
#else
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000).toUTC().toString("H:mm:ss");
#endif
            }
            else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000, Qt::UTC).toString("H:mm");
#else
                s = QDateTime::fromMSecsSinceEpoch(actSec * 1000).toUTC().toString("H:mm");
#endif
            }

            int textW = ViewerUtil::textWidth(fm_, s);
            painter->setFont(font_);

            if (xp - textW / 2 < rect.x()) {
                painter->drawText(
                    QRect(rect.x() + 1, timeTextY, textW, fm_.height()), Qt::AlignLeft | Qt::AlignVCenter, s);
            }
            else if (xp + textW / 2 + 2 >= rect.x() + rect.width()) {
                painter->drawText(QRect(rect.x() + rect.width() - textW - 1, timeTextY, textW, fm_.height()),
                                  Qt::AlignRight | Qt::AlignVCenter,
                                  s);
            }
            else {
                painter->drawText(
                    QRect(xp - textW / 2, timeTextY, textW, fm_.height()), Qt::AlignHCenter | Qt::AlignVCenter, s);
            }
        }
        // draw minor tick
        else {
            painter->drawLine(xp, minorTickTop, xp, minorTickBottom);
        }

        actSec += minorTick;
    }

    painter->restore();
}

int NodeTimelineHeader::secToPos(qint64 t, QRect rect) const {
    // qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period = (endTime_.msecsSinceStartOfDay() - startTime_.msecsSinceStartOfDay()) / 1000;
    return rect.x() +
           static_cast<int>(static_cast<float>(t) / static_cast<float>(period) * static_cast<float>(rect.width()));
}

// pos is based on the visible portion of the widget - scrollbars are ignored
QTime NodeTimelineHeader::posToTime(QPoint pos) const {
    int logicalIndex = logicalIndexAt(pos);
    if (logicalIndex == -1) {
        return {};
    }

    int xp = sectionPosition(logicalIndex);
    int w  = sectionSize(logicalIndex);

    // take the scrollbar into account
    QPoint rPos = realPos(pos);

    if (w <= 0 || rPos.x() < xp) {
        return {};
    }

    double r = static_cast<double>(rPos.x() - xp) / static_cast<double>(w);
    if (r < 0 || r > 1) {
        return {};
    }

    // qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period = endTime_.msecsSinceStartOfDay() / 1000 - startTime_.msecsSinceStartOfDay() / 1000;

    return startTime_.addSecs(r * static_cast<double>(period));
}

void NodeTimelineHeader::setStartTime(QTime t) {
    startTime_ = t;
    zoomHistory_.clear();
    zoomHistory_.push(QPair<QTime, QTime>(startTime_, endTime_));
    checkActionState();
    rerender();
}

void NodeTimelineHeader::setEndTime(QTime t) {
    endTime_ = t;
    zoomHistory_.clear();
    zoomHistory_.push(QPair<QTime, QTime>(startTime_, endTime_));
    checkActionState();
    rerender();
}

void NodeTimelineHeader::setPeriod(QTime t1, QTime t2) {
    zoomHistory_.clear();
    setPeriodCore(t1, t2, true);
}

void NodeTimelineHeader::setPeriodCore(QTime t1, QTime t2, bool addToHistory) {
    startTime_ = t1;
    endTime_   = t2;
    if (addToHistory) {
        zoomHistory_.push(QPair<QTime, QTime>(startTime_, endTime_));
    }
    checkActionState();
    rerender();
}

bool NodeTimelineHeader::isColumnZoomableAtIndex(int logicalIndex) const {
    return columnType_[logicalIndex] == DayColumn;
}

bool NodeTimelineHeader::isTimelineColumn(int logicalIndex) const {
    return columnType_[logicalIndex] == DayColumn;
}

bool NodeTimelineHeader::canBeZoomed() const {
    if (hasZoomableColumn()) {
        return (endTime_.msecsSinceStartOfDay() - startTime_.msecsSinceStartOfDay()) > 60 * 1000;
    }
    return false;
}
