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
#include <QTime>
#include <QWidget>

#include "ServerObserver.hpp"
#include "ServerComObserver.hpp"
#include "VProperty.hpp"

class QAction;
class QPainter;
class QTimer;

class PropertyMapper;
class ServerHandler;
class ServerUpdateData;


class ServerRefreshInfoWidget : public QWidget, public ServerObserver, public ServerComObserver,
                                public VPropertyObserver
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
    void slotTimeOutRefreshFinished();

protected:
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* e);
    void leaveEvent(QEvent*);
    void paintEvent(QPaintEvent*);

    void drawButton(QPainter*);
    void drawProgress(QPainter*);
    void adjustGeometry(bool);
    void updateSettings();
    void reloadAll();
    void fetchInfo();
    void adjustTimer(int toNext);
    void adjustToolTip();
    QString formatTime(int timeInSec) const;  
    bool isInButton(const QPoint& pos) const;
    bool isInText(const QPoint& pos) const;
    void printStatus() const;
    bool showCountdown() const {return showCountdownArc_ || showCountdownText_;}
    bool isActionEnabled() const;

    enum Component {ButtonComponent,TextComponent,NoComponent};
    enum Mode {NormalMode,FastMode,ContMode,ManualMode,NoMode};

    QAction* refreshAction_;
    ServerHandler* server_;
    QString serverName_;
    QString serverText_;
    QTimer *timer_;
    QTime inRefreshElapsed_;

    QFont font_;
    QFont fontTime_;
    QFont fontUpdate_;
    QFontMetrics fm_;
    QFontMetrics fmTime_;
    QFontMetrics fmUpdate_;

    static QIcon *icon_;
    static QBrush bgBrush_;
    static QPen borderPen_;
    static QPen disabledBorderPen_;
    static QBrush buttonBgHoverBrush_;
    static QPen buttonHoverPen_;
    static QBrush buttonBgRefreshBrush_;
    static QPen buttonRefreshPen_;
    static QBrush timeBgBrush_;
    static QBrush progBrush_;
    static QBrush progBgBrush_;
    static QPen textPen_;
    static QPen refreshTextPen_;
    static QPen disabledTextPen_;

    QRect buttonRect_;
    int buttonRadius2_;
    int timeTextLen_;
    int lastTextLen_;
    QRect serverRect_;
    QRect timeRect_;
    QRect progRect_;
    QRect lastRect_;
    Component currentComponent_;

    PropertyMapper* prop_;

    bool showCountdownArc_;
    bool showCountdownText_;
    Mode mode_;
    bool fastMode_;
    int contModeLimit_;
    int fastModeLimit_;
    bool hasInfo_;
    bool inRefresh_;
    bool userInitiatedRefresh_;
    bool showLastRefresh_;
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
