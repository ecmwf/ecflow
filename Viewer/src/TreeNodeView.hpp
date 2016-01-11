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
#include "VProperty.hpp"

class ActionHandler;
class Animation;
class ExpandNode;
class ExpandState;
class NodeFilterModel;
class PropertyMapper;
class TreeNodeModel;
class TreeNodeViewDelegate;

class TreeNodeView : public QTreeView, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
	explicit TreeNodeView(NodeFilterModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
	~TreeNodeView();

	void reload();
	void rerender();
	QWidget* realWidget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n);
	void selectFirstServer();
	void setModel(NodeFilterModel* model);

	void notifyChange(VProperty* p);

	void readSettings(VSettings* vs) {};

public Q_SLOTS:
	void slotSelectItem(const QModelIndex&);
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(VInfo_ptr,QString);
	void slotSetCurrent(VInfo_ptr);
	void slotSaveExpand(const VNode* node);
	void slotRestoreExpand(const VNode* node);
	void slotRepaint(Animation*);
	void slotRerender();
	void slotSizeHintChangedGlobal();

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);
	void infoPanelCommand(VInfo_ptr,QString);
	void dashboardCommand(VInfo_ptr,QString);

protected:
	QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);
	void saveExpand(ExpandNode *parentExpand,const QModelIndex& idx);
	void restoreExpand(ExpandNode *expand,const VNode* node);
	void adjustIndentation(int);
	void adjustBackground(QColor col);
	void expandAll(const QModelIndex& idx);
	void collapseAll(const QModelIndex& idx);

	ActionHandler* actionHandler_;
	ExpandState *expandState_;
	bool needItemsLayout_;
	int defaultIndentation_;
	TreeNodeViewDelegate* delegate_;
	PropertyMapper* prop_;
};

#endif



