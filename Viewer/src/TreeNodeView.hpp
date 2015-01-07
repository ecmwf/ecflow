//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TreeNodeView_HPP_
#define TreeNodeView_HPP_

#include <QTreeView>

#include "NodeViewBase.hpp"

#include "VInfo.hpp"

class ActionHandler;
class VConfig;
class NodeFilterModel;
class TreeNodeModel;

class TreeNodeView : public QTreeView, public NodeViewBase
{
Q_OBJECT

public:
	TreeNodeView(NodeFilterModel* model,QWidget *parent=0);
	void reload();
	QWidget* realWidget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n);

public Q_SLOTS:
	void slotSelectItem(const QModelIndex&);
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(std::vector<VInfo_ptr>,QString);

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);

protected:
	QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);
	ActionHandler* actionHandler_;
};

#endif



