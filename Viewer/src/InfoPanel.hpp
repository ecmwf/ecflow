//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANEL_HPP_
#define INFOPANEL_HPP_

#include <QDockWidget>
#include <QWidget>

#include "DashboardWidget.hpp"
#include "ServerObserver.hpp"
#include "Viewer.hpp"
#include "VInfo.hpp"

#include "ui_InfoPanel.h"

class QDockWidget;
class QTabWidget;
class InfoPanel;
class InfoPanelDef;
class InfoPanelItem;

class InfoPanelDock : public QDockWidget
{
public:
	InfoPanelDock(QString label,QWidget * parent=0);
	InfoPanel* infoPanel() const {return infoPanel_;}

protected:
	void showEvent(QShowEvent* event);
	void closeEvent (QCloseEvent *event);

	InfoPanel* infoPanel_;
	bool closed_;
};


class InfoPanelItemHandler
{
friend class InfoPanel;

public:
	InfoPanelItemHandler(InfoPanelDef* def,InfoPanelItem* item) :
		def_(def), item_(item) {}

	bool match(const std::vector<InfoPanelDef*>& defs) const;
	InfoPanelItem* item() const {return item_;}
	QWidget* widget();
	InfoPanelDef* def() const {return def_;}

protected:
		void addToTab(QTabWidget *);

private:
		InfoPanelDef* def_;
		InfoPanelItem* item_;
};


class InfoPanel : public DashboardWidget, public ServerObserver, public VInfoObserver, private Ui::InfoPanel
{
    Q_OBJECT

public:
    InfoPanel(QWidget* parent=0);
	virtual ~InfoPanel();
	bool frozen() const;
	bool detached() const;
	void detached(bool);
	void clear();

	//From DashboardWidget
	void reload() {};
	void rerender() {};
	void writeSettings(VSettings*);
	void readSettings(VSettings*);

	//From VInfoObserver
	void notifyDelete(VInfo*) {};
	void notifyDataLost(VInfo*);

	//From ServerObserver
	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
	void notifyServerDelete(ServerHandler* server);
	void notifyBeginServerClear(ServerHandler* server);
	void notifyEndServerClear(ServerHandler* server) {};
	void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {};
	void notifyEndServerScan(ServerHandler* server);
	void notifyServerConnectState(ServerHandler* server);
	void notifyServerSuiteFilterChanged(ServerHandler* server);

public Q_SLOTS:
	void slotReload(VInfo_ptr node);
	void slotCurrentWidgetChanged(int);
	void on_addTb_clicked();

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);

protected:
	void reset(VInfo_ptr node);
	void adjustInfo(VInfo_ptr node);
    void adjustTabs(VInfo_ptr node);
	InfoPanelItemHandler* findHandler(QWidget* w);
	InfoPanelItemHandler* findHandler(InfoPanelDef*);
	InfoPanelItem* findItem(QWidget* w);
	InfoPanelItemHandler* createHandler(InfoPanelDef*);

	QList<InfoPanelItemHandler*> items_;
	//std::map<InfoPanelDef*,InfoPanelItem*>
	VInfo_ptr info_;
};

#endif
