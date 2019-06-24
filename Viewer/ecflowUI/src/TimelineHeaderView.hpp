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

class QTreeView;

class TimelineHeader : public QHeaderView
{
Q_OBJECT

public:
    enum ColumnType {TimelineColumn,DayColumn,OtherColumn};

    explicit  TimelineHeader(QTreeView *view);

    QSize sizeHint() const;

    //void setStartDate(QDateTime);
    //void setEndDate(QDateTime);
    //void setPeriod(QDateTime t1,QDateTime t2);
    //QDateTime startDate() const {return startDate_;}
    //QDateTime endDate() const {return endDate_;}
    void setZoomActions(QAction* zoomInAction,QAction* zoomOutAction);
    //void setMaxDurations(int submittedDuration,int activeDuration);
    void viewModeChanged();

protected Q_SLOTS:
    void slotZoomState(bool);
    virtual void slotZoomOut(bool)=0;

Q_SIGNALS:
    void customButtonClicked(QString,QPoint);
    //void periodSelected(QDateTime,QDateTime);
    //void periodBeingZoomed(QDateTime,QDateTime);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    virtual void renderTimeline(const QRect& rect,QPainter* painter,int logicalIndex) const=0;

    QPoint realPos(QPoint pos) const;

    //void setPeriodCore(QDateTime t1,QDateTime t2,bool addToHistory);
    int secToPosPeriod(qint64 t,QRect rect,qint64 period) const;
    virtual int secToPos(qint64 t,QRect rect) const=0;

    //QDateTime posToDate(QPoint pos) const;
    //int dateToPos(QDateTime dt) const;

    virtual bool isTimelineColumn(int index) const=0;
    bool isColumnZoomable(QPoint pos) const;
    virtual bool isColumnZoomableAtIndex(int index) const=0;
    void enableZoomActions(bool b);
    virtual bool canBeZoomed() const=0;
    bool isZoomEnabled() const;
    void setZoomDisabled();
    virtual int zoomHistoryCount() const=0;
    virtual void doZoom()=0;
    virtual void beingZoomedCore()=0;

    //void doPeriodZoom();
    void checkActionState();
    bool hasTimeColumn() const;
    bool hasZoomableColumn() const;
    void rerender();

    QTreeView* view_;
    QList<ColumnType> columnType_;
    //QDateTime startDate_;
    //QDateTime endDate_;
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
    //QStack<QPair<QDateTime,QDateTime> > zoomHistory_;
    QCursor zoomCursor_;
    QAction* zoomInAction_;
    QAction* zoomOutAction_;

    //int submittedMaxDuration_;
    //int activeMaxDuration_;

    //QTime startTime_;
    //QTime endTime_;
};

#if 0
class MainTimelineHeader: public TimelineHeader
{
public:
    MainTimelineHeader(QTreeView *view);
};

class NodeTimelineHeader: public TimelineHeader
{
public:
      NodeTimelineHeader(QTreeView *view);
};
#endif

class MainTimelineHeader : public TimelineHeader
{
Q_OBJECT

public:
    explicit  MainTimelineHeader(QTreeView *view);

    void setStartDate(QDateTime);
    void setEndDate(QDateTime);
    void setPeriod(QDateTime t1,QDateTime t2);
    QDateTime startDate() const {return startDate_;}
    QDateTime endDate() const {return endDate_;}
    void setMaxDurations(int submittedDuration,int activeDuration);

protected Q_SLOTS:
    void slotZoomOut(bool);

Q_SIGNALS:
    void customButtonClicked(QString,QPoint);
    void periodSelected(QDateTime,QDateTime);
    void periodBeingZoomed(QDateTime,QDateTime);

protected:
    void renderTimeline(const QRect& rect,QPainter* painter,int logicalIndex) const;

    void setPeriodCore(QDateTime t1,QDateTime t2,bool addToHistory);
    int secToPos(qint64 t,QRect rect) const;
    QDateTime posToDate(QPoint pos) const;
    int dateToPos(QDateTime dt) const;

    void doZoom();
    void beingZoomedCore();
    bool isColumnZoomableAtIndex(int index) const;
    bool canBeZoomed() const;
    int zoomHistoryCount() const {return zoomHistory_.count();}
    bool isTimelineColumn(int index) const;

    QDateTime startDate_;
    QDateTime endDate_;
    QStack<QPair<QDateTime,QDateTime> > zoomHistory_;
    int submittedMaxDuration_;
    int activeMaxDuration_;
};

class NodeTimelineHeader : public TimelineHeader
{
Q_OBJECT

public:
    explicit NodeTimelineHeader(QTreeView *view);

    void setStartTime(QTime);
    void setEndTime(QTime);
    void setPeriod(QTime t1,QTime t2);
    QTime startTime() const {return startTime_;}
    QTime endTime() const {return endTime_;}

protected Q_SLOTS:
    void slotZoomOut(bool);

Q_SIGNALS:
    void customButtonClicked(QString,QPoint);
    void periodSelected(QTime,QTime);
    void periodBeingZoomed(QTime,QTime);

protected:
    void renderTimeline(const QRect& rect,QPainter* painter,int logicalIndex) const;
    void setPeriodCore(QTime t1,QTime t2,bool addToHistory);
    int secToPos(qint64 t,QRect rect) const;
    QTime posToTime(QPoint pos) const;
    int dateToPos(QDateTime dt) const;

    void doZoom();
    void beingZoomedCore();
    bool isColumnZoomableAtIndex(int index) const;
    bool canBeZoomed() const;
    int zoomHistoryCount() const {return zoomHistory_.count();}
    bool isTimelineColumn(int index) const;

    QStack<QPair<QTime,QTime> > zoomHistory_;
    QTime startTime_;
    QTime endTime_;
};



#endif // TIMELINEHEADERVIEW_HPP


