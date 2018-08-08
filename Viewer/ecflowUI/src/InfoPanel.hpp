//============================================================================
// Copyright 2009-2017 ECMWF.
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
    explicit InfoPanel(QWidget* parent=nullptr);
	~InfoPanel() override;
	bool frozen() const;
    void clear();
	void setCurrent(const std::string& name);
    void linkSelected(VInfo_ptr);
    void relayInfoPanelCommand(VInfo_ptr info,QString cmd);
    void relayDashboardCommand(VInfo_ptr info,QString cmd);

    void populateDialog() override;

    //From DashboardWidget
    void populateDockTitleBar(DashboardDockTitleWidget*) override;
    void reload() override {}
	void rerender() override;
    void writeSettings(VComboSettings*) override;
    void readSettings(VComboSettings*) override;
    void writeSettingsForDialog() override;
    void readSettingsForDialog() override;

	//From VInfoObserver
    void notifyDelete(VInfo*) override {}
	void notifyDataLost(VInfo*) override;

	//From ServerObserver
	void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) override;
	void notifyServerDelete(ServerHandler* server) override;
	void notifyBeginServerClear(ServerHandler* server) override;
    void notifyEndServerClear(ServerHandler* server) override {}
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) override {}
	void notifyEndServerScan(ServerHandler* server) override;
	void notifyServerConnectState(ServerHandler* server) override;
	void notifyServerSuiteFilterChanged(ServerHandler* server) override;
    void notifyEndServerSync(ServerHandler* server) override;

public Q_SLOTS:
	void slotReload(VInfo_ptr node);
	void setCurrentSelection(VInfo_ptr node) override {slotReload(node);}

protected Q_SLOTS:
    void slotReloadFromBc(VInfo_ptr node);
    void slotCurrentWidgetChanged(int);
    void on_actionBreadcrumbs__toggled(bool b);
    void on_actionFrozen__toggled(bool b);

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);

protected:
    void detachedChanged() override;

private:
    void localClear();
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
