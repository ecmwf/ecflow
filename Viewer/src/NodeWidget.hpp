/***************************** LICENSE START ***********************************

 Copyright 2015 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#ifndef NODEWIDGET_HPP_
#define NODEWIDGET_HPP_

#include <QString>
#include <QWidget>

#include "DashboardWidget.hpp"
#include "Viewer.hpp"
#include "VInfo.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class IconFilter;
class NodeFilter;
class NodeFilterModel;
class VModelData;
class NodePathWidget;
class NodeViewBase;
class ServerFilter;


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
	NodeWidget(QWidget* parent=0);
	virtual ~NodeWidget();

	AbstractNodeModel* model_;
	NodeFilterModel* filterModel_;
	NodeViewBase* view_;
	IconFilter* icons_;
	NodeFilter* filter_;
	VModelData* data_;

};

#endif
