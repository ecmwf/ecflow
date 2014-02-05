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

class ActionHandler;
class TreeNodeModel;

class TreeNodeView : public QTreeView, public NodeViewBase
{
Q_OBJECT

public:
		TreeNodeView(QString,QWidget *parent=0);
		//TreeNodeView::printDefTree(const std::string &server, int port);
		//TreeNodeView::printNode(node_ptr node, int indent, QTreeWidgetItem *parent)

public slots:
	void slotSelectItem(const QModelIndex&);
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);

protected:
		QModelIndexList selectedList();
		void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

		TreeNodeModel *model_;
		ActionHandler* actionHandler_;
};


#endif



