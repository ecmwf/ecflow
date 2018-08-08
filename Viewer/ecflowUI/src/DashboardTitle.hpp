//============================================================================
// Copyright 2009-2017 ECMWF.
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
	~DashboardTitle() override;

    Dashboard* dashboard() const {return dashboard_;}
    QString title() const {return title_;}
    QString tooltip() const {return tooltip_;}
    QString desc() const {return desc_;}
    QPixmap pix() const {return pix_;}
    QPixmap descPix() const {return descPix_;}
    void setMaxPixWidth(int w);
    void setCurrent(bool b);
    int fullWidth() const;

	void notifyServerFilterAdded(ServerItem* item) override;
	void notifyServerFilterRemoved(ServerItem* item) override;
	void notifyServerFilterChanged(ServerItem*) override;
	void notifyServerFilterDelete() override;

	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) override;
	void notifyServerDelete(ServerHandler* server) override;
    void notifyBeginServerClear(ServerHandler* server) override {}
	void notifyEndServerClear(ServerHandler* server) override;
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) override {}
	void notifyEndServerScan(ServerHandler* server) override;
	void notifyServerConnectState(ServerHandler* server) override;
	void notifyServerActivityChanged(ServerHandler* server) override;
    void notifyServerSuiteFilterChanged(ServerHandler* server) override {}

Q_SIGNALS:
    void changed(DashboardTitle*);

private:
	void clear();
    void updateTitle();

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
