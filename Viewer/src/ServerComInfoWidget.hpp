//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERCOMLINE_HPP
#define SERVERCOMLINE_HPP

#include <QBrush>
#include <QIcon>
#include <QPen>
#include <QPixmap>
#include <QWidget>

#include "ServerObserver.hpp"
#include "ServerComObserver.hpp"
#include "UiLog.hpp"
#include "VProperty.hpp"

class QAction;
class QPainter;
class QTimer;

class PropertyMapper;
class ServerHandler;
class ServerUpdateData;


class ServerRefreshInfoWidget : public QWidget, public ServerObserver, public ServerComObserver,
                                public VPropertyObserver,public UiLoggable
{
Q_OBJECT

public:
    explicit ServerRefreshInfoWidget(QAction* refreshAction,QWidget* parent=0);
    ~ServerRefreshInfoWidget();

    void setServer(ServerHandler* server);

    void notifyChange(VProperty* p);

    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {}
    void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler*);
    void notifyEndServerScan(ServerHandler*);
    void notifyServerActivityChanged(ServerHandler*);

    void notifyRefreshTimerStarted(ServerHandler* server);
    void notifyRefreshTimerStopped(ServerHandler* server);
    void notifyRefreshTimerChanged(ServerHandler* server);
    void notifyRefreshScheduled(ServerHandler* server);
    void notifyRefreshFinished(ServerHandler* server);

protected Q_SLOTS:
    void slotTimeOut();

protected:
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* e);
    void leaveEvent(QEvent*);
    void paintEvent(QPaintEvent*);

    void updateSettings();
    void fetchInfo();
    void adjustTimer(int toNext);
    void adjustToolTip();
    QString formatTime(int timeInSec) const;  
    bool isInButton(const QPoint& pos) const;
    bool isInText(const QPoint& pos) const;
    void printStatus() const;

    enum Component {ButtonComponent,TextComponent,NoComponent};

    QAction* refreshAction_;
    ServerHandler* server_;
    QFont font_;
    QFont fontTime_;
    QFontMetrics fm_;
    QFontMetrics fmTime_;
    QIcon icon_;
    QTimer *timer_;
    QBrush bgBrush_;
    QPen borderPen_;
    QBrush bgHoverBrush_;
    QPen borderHoverPen_;
    QPen arcPen_;
    QPen textPen_;
    QRect buttonRect_;
    int buttonRadius2_;
    Component currentComponent_;

    PropertyMapper* prop_;

    bool showCountdown_;
    bool fastMode_;
    bool hasInfo_;
    bool inRefresh_;
    QString lastRefresh_;
    QString nextRefresh_;
    int total_;
    int period_;
    int toNext_;

};

class ServerComActivityLine : public QWidget

{
//Q_OBJECT

public:
    explicit ServerComActivityLine(QWidget* parent=0);

    void setServer(ServerHandler* server);

protected:
    void paintEvent(QPaintEvent*);

    QFont font_;
    QFontMetrics fm_;
    ServerHandler* server_;
    QPixmap pix_;
    QTimer *timer_;
};

#if 0
class ServerRefreshInfoWidget : public QWidget
{
public:
    explicit ServerRefreshInfoWidget(QAction* refreshAction,QWidget *parent=0);

    void setServer(ServerHandler* server);

protected:
    QAction* refreshAction_;
    ServerComLineDisplay* infoW_;
};
#endif


#endif // SERVERCOMLINE_HPP
