//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef DASHBOARD_HPP_
#define DASHBOARD_HPP_

#include "Viewer.hpp"

#include <QSplitter>
#include <QMainWindow>

#include "ServerFilter.hpp"
#include "VInfo.hpp"
#include "VSettings.hpp"

#include <boost/property_tree/ptree.hpp>

class DashboardWidget;
class ServerFilter;
class ServerItem;
class VSettings;

class Dashboard : public QMainWindow, public ServerFilterObserver
{
    Q_OBJECT

public:
  	Dashboard(QString,QWidget* parent=0);
	~Dashboard();

	void reload();

	DashboardWidget* addWidget(const std::string& type);
	Viewer::ViewMode viewMode();
	void setViewMode(Viewer::ViewMode);
	ServerFilter* serverFilter() const {return serverFilter_;}
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n);

	//From  ServerFilterObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);

	void writeSettings(VSettings*);
	void readSettings(VSettings*);

public Q_SLOTS:
	//void slotFolderReplacedInView(Folder*);
	//void slotFolderChanged(Folder*);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
	void titleChanged(QWidget*,QString,QPixmap);

private:
	DashboardWidget* addWidget(const std::string& type,const std::string& dockId);
	void updateTitle();
	QString uniqueDockId();
	static std::string widgetSettingsId(int i);

	ServerFilter* serverFilter_;
	QList<DashboardWidget*> widgets_;
	static int maxWidgetNum_;
};


#endif
