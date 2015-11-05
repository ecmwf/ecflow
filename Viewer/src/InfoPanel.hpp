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

#include <QWidget>

#include "DashboardWidget.hpp"
#include "ServerObserver.hpp"
#include "VInfo.hpp"

#include "ui_InfoPanel.h"

class QMenu;
class QTabWidget;

class DashboardDockTitleWidget;
class InfoPanel;
class InfoPanelDef;
class InfoPanelItem;

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
    explicit InfoPanel(QWidget* parent=0);
	virtual ~InfoPanel();
	bool frozen() const;
	bool detached() const;
	void clear();
	void setCurrent(const std::string& name);
    void setDetached(bool);

    void populateDialog();

    //From DashboardWidget
    void populateDockTitleBar(DashboardDockTitleWidget*);
	void reload() {};
	void rerender();
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
	void notifyServerSyncFinished(ServerHandler* server);

public Q_SLOTS:
	void slotReload(VInfo_ptr node);
	void setCurrentSelection(VInfo_ptr node) {slotReload(node);}

protected Q_SLOTS:
    void slotReloadFromBc(VInfo_ptr node);
    void slotCurrentWidgetChanged(int);
    void on_actionBreadcrumbs__toggled(bool b);
    void on_actionFrozen__toggled(bool b);
    void on_actionDetached__toggled(bool b);

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);

private:
	void reset(VInfo_ptr node);
	void adjustInfo(VInfo_ptr node);
    void adjustTabs(VInfo_ptr node);
	InfoPanelItemHandler* findHandler(QWidget* w);
	InfoPanelItemHandler* findHandler(InfoPanelDef*);
	InfoPanelItem* findItem(QWidget* w);
	InfoPanelItemHandler* createHandler(InfoPanelDef*);
	void clearTab();
	void updateTitle();
    QMenu* buildOptionsMenu();

	QList<InfoPanelItemHandler*> items_;
	VInfo_ptr info_;
	bool tabBeingCleared_;
	bool tabBeingAdjusted_;
};

#endif
