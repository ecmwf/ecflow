//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TABLENODEVIEW_HPP_
#define TABLENODEVIEW_HPP_

#include <QTreeView>

#include "NodeViewBase.hpp"

#include "ViewNodeInfo.hpp"

class ActionHandler;
class ServerFilter;
class ViewFilter;
class TableNodeModel;
class TableNodeFilterModel;

class TableNodeView : public QTreeView, public NodeViewBase
{
Q_OBJECT

public:
	TableNodeView(QString,ServerFilter* serverFilter,ViewFilter*,QWidget *parent=0);
	void reload() {};
	QWidget* realWidget();

public slots:
	void slotSelectItem(const QModelIndex&);
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(std::vector<ViewNodeInfo_ptr>,QString);

signals:
	void selectionChanged(ViewNodeInfo_ptr);

protected:
	QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

	TableNodeModel *model_;
	TableNodeFilterModel* filterModel_;
	ActionHandler* actionHandler_;
};


#endif



