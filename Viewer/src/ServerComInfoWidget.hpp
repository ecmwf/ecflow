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

#include <QPixmap>
#include <QWidget>

class QAction;
class QPainter;
class QTimer;

class ServerHandler;
class ServerUpdateData;

class ServerComLineDisplay : public QWidget
{
//Q_OBJECT

public:
    explicit ServerComLineDisplay(QWidget* parent=0);

    void setServer(ServerHandler* server);


protected:
    void paintEvent(QPaintEvent*);

    void renderServerUpdate(QPainter* painter,const ServerUpdateData& data) const;
    QString formatTime(int timeInSec) const;
    QColor interpolate(QColor c1,QColor c2,float r) const;


    QFont font_;
    QFontMetrics fm_;
    ServerHandler* server_;
    QPixmap pix_;
    QTimer *timer_;
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


class ServerRefreshInfoWidget : public QWidget
{
public:
    explicit ServerRefreshInfoWidget(QAction* refreshAction,QWidget *parent=0);

    void setServer(ServerHandler* server);

protected:
    QAction* refreshAction_;
    ServerComLineDisplay* infoW_;
};



#endif // SERVERCOMLINE_HPP
