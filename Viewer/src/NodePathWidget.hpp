//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEPATHWIDGET_H
#define NODEPATHWIDGET_H

#include <QGraphicsView>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QToolButton>

#include "ViewNodeInfo.hpp"

#include <string>

class QAction;
class QHBoxLayout;
class QMenu;
class QSignalMapper;
class QToolButton;

class Node;
class NodePathWidget;

class NodePathItem : public QToolButton
{
public:
	enum Type {NodeType,MenuType,ElideType};
	NodePathItem(Type type,int index,QWidget* parent=0) : QToolButton(parent), type_(type), index_(index) {};
	int index() const {return index_;}

protected:
	Type type_;
	int index_;
};

class NodePathNodeItem : public NodePathItem
{
public:
	NodePathNodeItem(int index,QString name,QColor col,bool current,QWidget* parent=0);
	void reset(QString name,QColor col,bool selected);
    void colour(QColor col);
    void current(bool);

protected:
    void resetStyle();
    bool isDark(QColor col) const;

    QString qss_;
	QColor col_;
	bool current_;
};

class NodePathServerItem : public NodePathNodeItem
{
public:
	NodePathServerItem(int index,QString name,QColor col,bool current,QWidget* parent=0);
};

class NodePathMenuItem : public NodePathItem
{
public:
	NodePathMenuItem(int index,QWidget* parent=0);
};


class NodePathWidget : public QWidget
{
Q_OBJECT

public:
	NodePathWidget(QWidget* parent=0);
	~NodePathWidget();

public slots:
	void setPath(QString);
	void setPath(ViewNodeInfo_ptr);

protected slots:
	void slotContextMenu(const QPoint&);
	void nodeItemSelected();
	void menuItemSelected();

signals:
	void selected(ViewNodeInfo_ptr);

protected:
	void clearLayout();
	void clearContents();
	void loadMenu(const QPoint& pos,ViewNodeInfo_ptr p);
	ViewNodeInfo_ptr nodeAt(int);
	QList<Node*> toNodeList(ViewNodeInfo_ptr info);
	int findInPath(ViewNodeInfo_ptr p1,ViewNodeInfo_ptr p2,bool sameServer);
	void infoIndex(int idx);
	void paintEvent(QPaintEvent *);

	QList<NodePathNodeItem*> nodeItems_;
	QList<NodePathMenuItem*> menuItems_;

	QHBoxLayout* layout_;
	ViewNodeInfo_ptr info_;
	ViewNodeInfo_ptr infoFull_;
	int infoIndex_;

	//When it is set to "true": if a node item (i.e. not a menu) is selected the breadcrumbs
	//will stay the same and only will change the current node/selection!
	bool stayInParent_;
};

#endif
