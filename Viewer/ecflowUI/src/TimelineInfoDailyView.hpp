//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEINFODIALYVIEW_HPP
#define TIMELINEINFODIALYVIEW_HPPP

#include <QDialog>
#include <QSettings>
#include <QTreeView>
#include <QWidget>
#include <QSettings>
#include <QAbstractItemModel>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "TimelineData.hpp"
#include "VProperty.hpp"

class PropertyMapper;
class TimelineData;
class TimelineItem;
class VNState;
class NodeTimelineHeader;


class TimelineInfoDailyModel : public QAbstractItemModel
{
public:
    explicit TimelineInfoDailyModel(QObject *parent=0);
    ~TimelineInfoDailyModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    TimelineItem* data() const {return data_;}
    void load(TimelineItem*,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                 unsigned int endDateSec);
    void clearData();
    bool hasData() const;
    unsigned int endDateSec() const {return endDateSec_;}

protected:
    TimelineItem* data_;
    std::vector<unsigned int> days_;
    unsigned int viewStartDateSec_;
    unsigned int viewEndDateSec_;
    unsigned int endDateSec_;
};

class TimelineInfoDailyDelegate : public QStyledItemDelegate, public VPropertyObserver
{

public:
    explicit TimelineInfoDailyDelegate(TimelineInfoDailyModel* model,QWidget *parent);
    ~TimelineInfoDailyDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    void notifyChange(VProperty* p);

    void setStartTime(QTime);
    void setEndTime(QTime);
    void setPeriod(QTime t1,QTime t2);
    //void setMaxDurations(int submittedDuration,int activeDuration);

//Q_SIGNALS:
//    void sizeHintChangedGlobal();

protected:
    void updateSettings();
    void renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,const QModelIndex& index) const;
    void drawCell(QPainter *painter,QRect r,QColor fillCol,bool hasGrad,bool lighter) const;
    int timeToPos(QRect r,unsigned int time) const;

    TimelineInfoDailyModel* model_;
    PropertyMapper* prop_;
    QFont font_;
    QFontMetrics fm_;
    QPen borderPen_;
    int topPadding_;
    int bottomPadding_;
    QTime startTime_;
    QTime endTime_;

    //int submittedMaxDuration_;
    //int activeMaxDuration_;
    //int durationMaxTextWidth_;
};

class TimelineInfoDailyView : public QTreeView
{
Q_OBJECT

public:
    explicit TimelineInfoDailyView(QWidget *parent=0);
    ~TimelineInfoDailyView();

    void rerender();

    void setStartTime(QTime);
    void setEndTime(QTime);
    void setPeriod(QTime t1,QTime t2);
    void setZoomActions(QAction* zoomInAction,QAction* zoomOutAction);
    void load(TimelineItem *data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                                    unsigned int endDateSec);

    void readSettings(QSettings&);
    void writeSettings(QSettings&);

protected Q_SLOTS:
    //void slotDoubleClickItem(const QModelIndex&);
    //void slotContextMenu(const QPoint &position);
    //void slotHeaderContextMenu(const QPoint &position);
    //void slotSizeHintChangedGlobal();
    //void slotRerender();
    void periodSelectedInHeader(QTime t1,QTime t2);
    //void slotHzScrollbar(int,int);
    //void adjustHeader();

Q_SIGNALS:
    void periodSelected(QTime,QTime);
    void periodBeingZoomed(QTime,QTime);

protected:
    //QModelIndexList selectedList();
    //void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);
    //void adjustBackground(QColor col);
    //void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    //void setSortingEnabledNoExec(bool b);
    //void showDetails(const QModelIndex& indexClicked);
    //void lookup(const QModelIndex&);
    //void copyPath(const QModelIndex&);
    //void updateDurations();
    //int computeMaxDuration(QString);

    TimelineInfoDailyModel* model_;
    NodeTimelineHeader* header_;
    bool headerBeingAdjusted_;
    TimelineInfoDailyDelegate *delegate_;
    bool needItemsLayout_;
    bool setCurrentIsRunning_;
    QDateTime startDate_;
    QDateTime endDate_;
    QTime startTime_;
    QTime endTime_;
};


#endif // TIMELINEINFODAILYVIEW_HPP
