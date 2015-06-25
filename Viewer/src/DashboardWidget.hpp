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

class VSettings;

class DashboardWidget : public QWidget
{
public:
	explicit DashboardWidget(QWidget* parent=0) : QWidget(parent) {};
	virtual ~DashboardWidget() {};

	virtual void reload()=0;
	virtual void rerender()=0;
	virtual void writeSettings(VSettings*)=0;
	virtual void readSettings(VSettings*)=0;

	void id(const std::string& id) {id_=id;}

protected:
	std::string id_;
};


/*
class DashboardDock : public QDockWidget
{
public:
	DashboardDock(QString label,DashboardWidget* dw,QWidget * parent=0);
	DashboardWidget* dashboardWidget() const {return dw_;}

protected:
	void showEvent(QShowEvent* event);
	void closeEvent (QCloseEvent *event);

	DashboardWidget* dw_;
	bool closed_;
};




DashboardDock::DashboardDock(QString label,DashboardWidget* dw,QWidget * parent) :
   QDockWidget(label,parent),
   dw_(dw),
   closed_(true)
{
	//setAllowedAreas(Qt::BottomDockWidgetArea |
	//			Qt::RightDockWidgetArea);

	//setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
	//			QDockWidget::DockWidgetFloatable);

	//_=new InfoPanel(parent);

	//Just to be sure that init is correct


	setWidget(dw_);
}

void DashboardDock::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void DashboardDock::closeEvent (QCloseEvent *event)
{
	QWidget::closeEvent(event);

	//closed_=true;
	//infoPanel_->clear();
	//infoPanel_->setEnabled(false);
}

*/


#endif
