/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#ifndef NODEVIEWHANDLER_HPP_
#define NODEVIEWHANDLER_HPP_

#include <map>
#include <QString>
#include <QWidget>

#include "Viewer.hpp"
#include "VInfo.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class NodeFilterModel;
class NodeViewBase;
class VConfig;
class VSettings;

class DashboardWidget : public QWidget
{
public:
	DashboardWidget(QWidget* parent=0) : QWidget(parent) {};
	virtual ~DashboardWidget() {};
	virtual void reload()=0;
	virtual void writeSettings(VSettings*)=0;
	virtual void readSettings(VSettings*)=0;

	void id(const std::string& id) {id_=id;}

protected:
	std::string id_;
};


class NodeWidget : public DashboardWidget
{
public:
	void active(bool);
	bool active() const;
	NodeViewBase* view() const {return view_;}
	QWidget* widget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr info);
	void reload();

protected:
	NodeWidget(QWidget* parent=0): DashboardWidget(parent), model_(0), filterModel_(0), view_(0) {};

	AbstractNodeModel* model_;
	NodeFilterModel* filterModel_;
	NodeViewBase* view_;
};

class TreeNodeWidget : public NodeWidget
{
public:
	TreeNodeWidget(VConfig*,QWidget* parent=0);
	void writeSettings(VSettings*);
	void readSettings(VSettings*);
};

class TableNodeWidget : public NodeWidget
{
public:
	TableNodeWidget(VConfig*,QWidget* parent=0);
	void writeSettings(VSettings*);
	void readSettings(VSettings*);
};

class NodeViewHandler
{
public:
    NodeViewHandler(QStackedLayout*);

	void add(Viewer::ViewMode,NodeWidget*);
	Viewer::ViewMode currentMode() const {return currentMode_;}
	bool setCurrentMode(Viewer::ViewMode);
	bool setCurrentMode(int);
	NodeWidget* currentControl() const {return control(currentMode_);}

private:
	NodeWidget* control(Viewer::ViewMode) const;

	Viewer::ViewMode  currentMode_;
  	std::map<Viewer::ViewMode,NodeWidget*> controls_;
	std::map<Viewer::ViewMode,int> indexes_;
	QStackedLayout* stacked_;
};

#endif
