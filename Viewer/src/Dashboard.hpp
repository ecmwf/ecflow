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

#include <QMainWindow>

#include "ServerFilter.hpp"
#include "VInfo.hpp"
#include "VSettings.hpp"

class DashboardTitle;
class DashboardWidget;
class ServerFilter;
class ServerItem;
class VComboSettings;

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
  	Dashboard(QString,QWidget* parent=0);
	~Dashboard();

	void reload();
	void rerender();

	DashboardWidget* addWidget(const std::string& type);
	DashboardWidget* addDialog(const std::string& type);
	Viewer::ViewMode viewMode();
	void setViewMode(Viewer::ViewMode);
	ServerFilter* serverFilter() const {return serverFilter_;}
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n);
	void selectFirstServer();

	void writeSettings(VComboSettings*);
	void readSettings(VComboSettings*);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
	void titleChanged(QWidget*,QString,QPixmap);

public Q_SLOTS:
	void slotPopInfoPanel(VInfo_ptr,QString);

protected Q_SLOTS:
	void slotTitle(QString,QPixmap);
    void slotDockClose();
    void slotDialogFinished(int);
    void slotPopInfoPanel(QString);

private:
	DashboardWidget* addWidgetCore(const std::string& type);
	DashboardWidget* addWidget(const std::string& type,const std::string& dockId);
	QString uniqueDockId();
	static std::string widgetSettingsId(int i);
	void selectFirstServerInView();
	VInfo_ptr currentSelectionInView();

	ServerFilter* serverFilter_;
	DashboardTitle* titleHandler_;
	QList<DashboardWidget*> widgets_;
	static int maxWidgetNum_;
};


#endif
