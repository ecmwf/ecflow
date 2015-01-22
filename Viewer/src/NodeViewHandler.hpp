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

#include "DashboardWidget.hpp"
#include "Viewer.hpp"
#include "VInfo.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class NodeFilterModel;
class NodePathWidget;
class NodeViewBase;
class VConfig;
class VSettings;

class NodeWidget : public DashboardWidget
{
Q_OBJECT

public:
	void active(bool);
	bool active() const;
	NodeViewBase* view() const {return view_;}
	QWidget* widget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr info);
	void reload();

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);

protected:
	NodeWidget(QWidget* parent=0): DashboardWidget(parent), model_(0), filterModel_(0), view_(0), bc_(0) {};

	AbstractNodeModel* model_;
	NodeFilterModel* filterModel_;
	NodeViewBase* view_;
	NodePathWidget* bc_;
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

#endif
