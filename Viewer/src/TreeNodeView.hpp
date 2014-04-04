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
#include <QStyledItemDelegate>

#include "NodeViewBase.hpp"

#include "ViewNodeInfo.hpp"

class ActionHandler;
class FilterData;
class TreeNodeModel;
class TreeNodeFilterModel;


class TreeNodeViewDelegate : public QStyledItemDelegate
{
public:
	TreeNodeViewDelegate(QWidget *parent=0);
	void paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const;
};


class TreeNodeView : public QTreeView, public NodeViewBase
{
Q_OBJECT

public:
	TreeNodeView(QString,FilterData*,QWidget *parent=0);
	void reload();
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

	TreeNodeModel *model_;
	TreeNodeFilterModel *filterModel_;
	ActionHandler* actionHandler_;
};


#endif



