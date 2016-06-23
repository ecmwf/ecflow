//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef DASHBOARDTITLE_HPP_
#define DASHBOARDTITLE_HPP_

#include <QObject>
#include <QPixmap>

#include "ServerFilter.hpp"
#include "ServerObserver.hpp"

class Dashboard;
class ServerItem;

class DashboardTitle : public QObject, public ServerFilterObserver, public ServerObserver
{
Q_OBJECT

friend class Dashboard;

public:
    DashboardTitle(ServerFilter*,Dashboard *parent);
	~DashboardTitle();

    Dashboard* dashboard() const {return dashboard_;}
    QString title() const {return title_;}
    QString tooltip() const {return tooltip_;}
    QString desc() const {return desc_;}
    QPixmap pix() const {return pix_;}
    QPixmap descPix() const {return descPix_;}
    void setMaxPixWidth(int w);
    void setCurrent(bool b);

	void notifyServerFilterAdded(ServerItem* item);
	void notifyServerFilterRemoved(ServerItem* item);
	void notifyServerFilterChanged(ServerItem*);
	void notifyServerFilterDelete();

	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
	void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server) {}
	void notifyEndServerClear(ServerHandler* server);
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
	void notifyEndServerScan(ServerHandler* server);
	void notifyServerConnectState(ServerHandler* server);
	void notifyServerActivityChanged(ServerHandler* server);
    void notifyServerSuiteFilterChanged(ServerHandler* server) {}

Q_SIGNALS:
    void changed(DashboardTitle*);

private:
	void clear();
    void updateTitle();
    //void drawServerRect(QPainter *,QColor,QRect,QString,QColor,QRect);

    Dashboard* dashboard_;
	ServerFilter* filter_;
    int maxPixWidth_;
    QPixmap pix_;
    QPixmap descPix_;
    QString title_;
    QString tooltip_;
    QString desc_;
    bool current_;
    static int lighter_;
};

#endif
