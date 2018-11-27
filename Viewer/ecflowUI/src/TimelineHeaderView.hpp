//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMELINEHEADERVIEW_HPP
#define TIMELINEHEADERVIEW_HPP

#include <QAction>
#include <QHeaderView>
#include <QPen>
#include <QDateTime>
#include <QStack>

class TimelineHeader : public QHeaderView
{
Q_OBJECT

public:
    explicit  TimelineHeader(QWidget *parent);

    QSize sizeHint() const;

    void setStartDate(QDateTime);
    void setEndDate(QDateTime);
    void setPeriod(QDateTime t1,QDateTime t2);
    QDateTime startDate() const {return startDate_;}
    QDateTime endDate() const {return endDate_;}
    void setZoomActions(QAction* zoomInAction,QAction* zoomOutAction);
    void setMaxDurations(int submittedDuration,int activeDuration);
    void viewModeChanged();

protected Q_SLOTS:
    void slotZoomState(bool);
    void slotZoomOut(bool);

Q_SIGNALS:
    void customButtonClicked(QString,QPoint);
    void periodSelected(QDateTime,QDateTime);
    void periodBeingZoomed(QDateTime,QDateTime);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void renderTimeline(const QRect& rect,QPainter* painter) const;
    void renderDuration(const QRect& rect,QPainter* painter,QBrush bgBrush,int period) const;
    int secToPos(qint64 t,QRect rect,qint64 period) const;

    void setPeriodCore(QDateTime t1,QDateTime t2,bool addToHistory);
    int secToPos(qint64 t,QRect rect) const;
    QDateTime posToDate(QPoint pos) const;
    int dateToPos(QDateTime dt) const;
    bool canBeZoomed() const;
    bool isZoomEnabled() const;
    void setZoomDisabled();
    qint64 zoomPeriodInSec(QPoint startX,QPoint endX) const;
    void checkActionState();
    bool inTimelineColumn(QPoint pos) const;
    bool hasTimelineColumn() const;

    QDateTime startDate_;
    QDateTime endDate_;
    QFont font_;
    QFontMetrics fm_;
    QColor timelineCol_;
    QColor dateTextCol_;
    QColor timeTextCol_;
    QColor timelineFrameBorderCol_;
    QBrush timelineBrush_;
    //int timelineSection_;
    int timelineFrameSize_;
    int majorTickSize_;

    QColor zoomCol_;
    QPoint zoomStartPos_;
    QPoint zoomEndPos_;
    bool inZoom_;
    QStack<QPair<QDateTime,QDateTime> > zoomHistory_;
    QCursor zoomCursor_;
    QAction* zoomInAction_;
    QAction* zoomOutAction_;

    int submittedMaxDuration_;
    int activeMaxDuration_;
};

#endif // TIMELINEHEADERVIEW_HPP


