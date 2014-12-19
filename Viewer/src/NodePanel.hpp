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

class NodeWidget;
class VConfig;

class NodePanel : public TabWidget
{
    Q_OBJECT

public:
	NodePanel(QWidget* parent=0);
	virtual ~NodePanel();

	void setViewMode(Viewer::ViewMode);
	Viewer::ViewMode viewMode();

	VConfig* config();

	NodeWidget* currentNodeWidget();
	void addWidget();
	void resetWidgets(QStringList);
	void reload();
	VInfo_ptr currentSelection();

	void save(boost::property_tree::ptree &pt);
	void load(const boost::property_tree::ptree &pt);

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
	NodeWidget* addWidget(QString);
	void tabBarCommand(QString, int);
	NodeWidget* nodeWidget(int index);
};

#endif
