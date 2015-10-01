//============================================================================
// Copyright 2014 ECMWF.
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
	DashboardTitle(ServerFilter*,QObject *parent=0);
	~DashboardTitle();

	void notifyServerFilterAdded(ServerItem* item);
	void notifyServerFilterRemoved(ServerItem* item);
	void notifyServerFilterChanged(ServerItem*);
	void notifyServerFilterDelete();

	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
	void notifyServerDelete(ServerHandler* server);
	void notifyBeginServerClear(ServerHandler* server) {};
	void notifyEndServerClear(ServerHandler* server);
	void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {};
	void notifyEndServerScan(ServerHandler* server);
	void notifyServerConnectState(ServerHandler* server);
	void notifyServerActivityChanged(ServerHandler* server);
	void notifyServerSuiteFilterChanged(ServerHandler* server) {};

Q_SIGNALS:
	void changed(QString,QPixmap);

private:
	void updateTitle();

	ServerFilter* filter_;
};

#endif
