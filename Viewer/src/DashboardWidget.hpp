/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#ifndef DASHBOARDWIDGET_HPP_
#define DASHBOARDWIDGET_HPP_

#include <string>
#include <QDockWidget>
#include <QWidget>
#include <QAction>

#include "VInfo.hpp"

class DashboardDockTitleWidget;
class ServerFilter;
class VSettings;

class DashboardWidget : public QWidget
{
Q_OBJECT

public:
	DashboardWidget(const std::string& type, QWidget* parent=0) :
		QWidget(parent),type_(type), acceptSetCurrent_(false) {};
	virtual ~DashboardWidget() {};

    virtual void populateDockTitleBar(DashboardDockTitleWidget*)=0;
    virtual void populateDialog()=0;
    virtual void reload()=0;
	virtual void rerender()=0;
	virtual bool selectFirstServerInView() {return false;};
	virtual VInfo_ptr currentSelection() {return VInfo_ptr(); }
	virtual QList<QAction*> dockTitleActions() {return QList<QAction*>();}

	virtual void writeSettings(VSettings*)=0;
	virtual void readSettings(VSettings*)=0;

	const std::string type() const {return type_;}
	void id(const std::string& id) {id_=id;}

public Q_SLOTS:
	virtual void setCurrentSelection(VInfo_ptr)=0;

Q_SIGNALS:
	void titleUpdated(QString);

protected:
	std::string id_;
	std::string type_;
	bool acceptSetCurrent_;
};

#endif
