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

#include <QHeaderView>
#include <QTreeView>

#include "NodeViewBase.hpp"

#include "VInfo.hpp"

class QComboBox;

class ActionHandler;
class TableNodeModel;
class NodeFilterModel;
class NodeFilterDef;

class TableNodeView : public QTreeView, public NodeViewBase
{
Q_OBJECT

public:
	explicit TableNodeView(NodeFilterModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
	void reload() {};
	void rerender() {};
	QWidget* realWidget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n) {};
	void selectFirstServer() {}
	void setModel(NodeFilterModel *model);

	void readSettings(VSettings* vs);

public Q_SLOTS:
	void slotSelectItem(const QModelIndex&);
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(std::vector<VInfo_ptr>,QString);
	void slotHeaderContextMenu(const QPoint &position);

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);
	void infoPanelCommand(VInfo_ptr,QString);

protected:
	QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

	ActionHandler* actionHandler_;
};

class TableNodeHeader : public QHeaderView
{
Q_OBJECT

public:
	explicit TableNodeHeader(QWidget *parent=0);

	QSize sizeHint() const;

public Q_SLOTS:
	void slotSectionResized(int i);

protected:
	void showEvent(QShowEvent *QSize);

	QMap<int, QComboBox *> combo_;
};

#endif



