//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include <QIcon>

#include <boost/property_tree/ptree.hpp>

class Dashboard;
class DashboardTitle;
class ServerFilter;
class VComboSettings;

class NodePanel : public TabWidget
{
    Q_OBJECT

public:
	explicit NodePanel(QWidget* parent=nullptr);
	virtual ~NodePanel();

	void setViewMode(Viewer::ViewMode);
	Viewer::ViewMode viewMode();

	ServerFilter* serverFilter();

	Dashboard* currentDashboard();
	void addWidget();
	void resetWidgets(QStringList);
	void reload();
	void rerender();
	void refreshCurrent();
	void resetCurrent();
	VInfo_ptr currentSelection();
    bool selectInTreeView(VInfo_ptr);
	void addToDashboard(const std::string& type);
	void init();
	void openDialog(VInfo_ptr,const std::string& type);
	void addSearchDialog();

	void writeSettings(VComboSettings*);
	void readSettings(VComboSettings*);

public Q_SLOTS:
	void slotCurrentWidgetChanged(int);
    void slotSelection(VInfo_ptr);
    void slotNewTab();
    void slotSelectionChangedInWidget(VInfo_ptr);

protected Q_SLOTS:
    void slotTabRemoved();
    void slotTabTitle(DashboardTitle* w);

Q_SIGNALS:
	void itemInfoChanged(QString);
	void currentWidgetChanged();
    void selectionChangedInCurrent(VInfo_ptr);
	void contentsChanged();

protected:
    void resizeEvent(QResizeEvent *e);
    void adjustTabTitle();
    int tabAreaWidth() const;

    Dashboard* addWidget(QString);
	void tabBarCommand(QString, int);
	Dashboard* nodeWidget(int index);
	static std::string tabSettingsId(int i);
};

#endif
