//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMELINEVIEW_HPP
#define TIMELINEVIEW_HPP

#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QPen>
#include <QDateTime>

#include "VInfo.hpp"
#include "VProperty.hpp"

class ActionHandler;
class PropertyMapper;
class TimelineHeader;
class TimelineModel;
class VSettings;

class TimelineDelegate : public QStyledItemDelegate, public VPropertyObserver
{
 Q_OBJECT

public:
    explicit TimelineDelegate(TimelineModel* model,QWidget *parent=0);
    ~TimelineDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    void notifyChange(VProperty* p);

Q_SIGNALS:
    void sizeHintChangedGlobal();

protected:
    void updateSettings();
    void renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,int) const;
    int timeToPos(QRect r,unsigned int time) const;

    //void renderNode(QPainter *painter,const QModelIndex& index,
    //                const QStyleOptionViewItem& option,QString text) const;

    //ModelColumn* columns_;
    TimelineModel* model_;
    PropertyMapper* prop_;
    QFont font_;
    QPen borderPen_;
};


class TimelineView : public QTreeView,public VPropertyObserver
{
Q_OBJECT

public:
    explicit TimelineView(TimelineModel* model,QWidget *parent=0);
    ~TimelineView();

    void rerender();

    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);

    void notifyChange(VProperty* p);

    void readSettings(VSettings*);
    void writeSettings(VSettings*);

public Q_SLOTS:
    void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint &position);
    void slotViewCommand(VInfo_ptr,QString);
    void slotHeaderContextMenu(const QPoint &position);
    void slotSizeHintChangedGlobal();
    void slotRerender();
    void setStartDate(QDateTime);
    void setEndDate(QDateTime);
    void setPeriod(QDateTime t1,QDateTime t2);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);
    //void headerButtonClicked(QString,QPoint);

protected:
    QModelIndexList selectedList();
    void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);
    void adjustBackground(QColor col);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void setSortingEnabledNoExec(bool b);

    TimelineModel* model_;
    ActionHandler* actionHandler_;
    TimelineHeader* header_;
    bool needItemsLayout_;
    PropertyMapper* prop_;
    bool setCurrentIsRunning_;
};


class TimelineHeader : public QHeaderView
{
Q_OBJECT

public:
    explicit TimelineHeader(QWidget *parent=0);

    QSize sizeHint() const;
    void setModel(QAbstractItemModel *model);

    void setStartDate(QDateTime);
    void setEndDate(QDateTime);
    void setPeriod(QDateTime t1,QDateTime t2);

public Q_SLOTS:
    void slotSectionResized(int i);

Q_SIGNALS:
    void customButtonClicked(QString,QPoint);

protected:
    void showEvent(QShowEvent *QSize);
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    void mousePressEvent(QMouseEvent *event);

    int secToPos(qint64 t,QRect rect) const;

    QDateTime startDate_;
    QDateTime endDate_;
    QPixmap customPix_;
    QFont font_;
    QFontMetrics fm_;

    //mutable QMap<int,TableNodeHeaderButton> customButton_;
};

#endif // TIMELINEVIEW_HPP


