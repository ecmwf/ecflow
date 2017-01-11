//============================================================================
// Copyright 2016 ECMWF.
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
#include "VProperty.hpp"

class QComboBox;

class ActionHandler;
class TableNodeModel;
class TableNodeSortModel;
class NodeFilterDef;
class PropertyMapper;
class TableNodeHeader;

class TableNodeView : public QTreeView, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:

	explicit TableNodeView(TableNodeSortModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
    ~TableNodeView();

    void reload() {}
	void rerender();
	QWidget* realWidget();
	VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
	void selectFirstServer() {}
	void setModel(TableNodeSortModel *model);

	void notifyChange(VProperty* p);

    void readSettings(VSettings*);
    void writeSettings(VSettings*);

public Q_SLOTS:
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(std::vector<VInfo_ptr>,QString);
	void slotHeaderContextMenu(const QPoint &position);
	void slotSizeHintChangedGlobal();
    void slotRerender();
    void slotViewCommand(VInfo_ptr,QString) {}

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);
	void infoPanelCommand(VInfo_ptr,QString);
	void dashboardCommand(VInfo_ptr,QString);
	void headerButtonClicked(QString,QPoint);

protected:
	QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);
	void adjustBackground(QColor col);
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    TableNodeSortModel* model_;
	ActionHandler* actionHandler_;
	TableNodeHeader* header_;
	bool needItemsLayout_;
	PropertyMapper* prop_;
    bool setCurrentIsRunning_;
};

class TableNodeHeaderButton
{
public:
	TableNodeHeaderButton(QString id) : id_(id) {}

	QString id() const {return id_;}
	void setRect(QRect r) {rect_=r;}
	QRect rect() const {return rect_;}

	QString id_;
	QRect rect_;
};

class TableNodeHeader : public QHeaderView
{
Q_OBJECT

public:
	explicit TableNodeHeader(QWidget *parent=0);

	QSize sizeHint() const;
	void setModel(QAbstractItemModel *model);

public Q_SLOTS:
	void slotSectionResized(int i);

Q_SIGNALS:
	void customButtonClicked(QString,QPoint);

protected:
	void showEvent(QShowEvent *QSize);
	void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
	void mousePressEvent(QMouseEvent *event);

	QPixmap customPix_;
	mutable QMap<int,TableNodeHeaderButton> customButton_;
};

#endif



