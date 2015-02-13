//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEPANEL_HPP_
#define NODEPANEL_HPP_

#include "Viewer.hpp"
#include "TabWidget.hpp"
#include "VInfo.hpp"

#include <boost/property_tree/ptree.hpp>

class Dashboard;
class ServerFilter;
class VSettings;

class NodePanel : public TabWidget
{
    Q_OBJECT

public:
	NodePanel(QWidget* parent=0);
	virtual ~NodePanel();

	void setViewMode(Viewer::ViewMode);
	Viewer::ViewMode viewMode();

	ServerFilter* serverFilter();

	Dashboard* currentNodeWidget();
	void addWidget();
	void resetWidgets(QStringList);
	void reload();
	VInfo_ptr currentSelection();
	void addToDashboard(const std::string& type);

	void writeSettings(VSettings*);
	void readSettings(VSettings*);

public Q_SLOTS:
	void slotCurrentWidgetChanged(int);
	void slotSelection(VInfo_ptr);

	//void slotIconCommand(QString,IconObjectH);
	//void slotDesktopCommand(QString,QPoint);
	void slotNewTab();
	//void slotNewWindow(bool);

Q_SIGNALS:
	void itemInfoChanged(QString);
	void currentWidgetChanged();
	void selectionChanged(VInfo_ptr);

protected:
	Dashboard* addWidget(QString);
	void tabBarCommand(QString, int);
	Dashboard* nodeWidget(int index);
	static std::string tabSettingsId(int i);
};

#endif
