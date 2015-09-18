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

#include "NodeObserver.hpp"
#include "ServerObserver.hpp"
#include "VInfo.hpp"

#include <string>

class QAction;
class QHBoxLayout;
class QMenu;
class QSignalMapper;
class QToolButton;

class Node;
class NodePathWidget;
class VSettings;

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
	NodePathNodeItem(int index,QString name,QColor col,QColor fontCol,bool current,QWidget* parent=0);
	void reset(QString name,QColor col,QColor fontCol,bool selected);
	void reset(QString name,QColor col,QColor fontCol);
    void colour(QColor col);
    void current(bool);

protected:
    void resetStyle();
    bool isDark(QColor col) const;

    QString qss_;
    QString qssSelected_;
	QColor col_;
	QColor fontCol_;
	bool current_;
};

class NodePathServerItem : public NodePathNodeItem
{
public:
	NodePathServerItem(int index,QString name,QColor col,QColor,bool current,QWidget* parent=0);
};

class NodePathMenuItem : public NodePathItem
{
public:
	NodePathMenuItem(int index,QWidget* parent=0);
};


class NodePathWidget : public QWidget, public NodeObserver, public ServerObserver
{
Q_OBJECT

public:
	explicit NodePathWidget(QWidget* parent=0);
	~NodePathWidget();

	bool active() const {return active_;}
	void active(bool);
	void clear(bool detachObservers=true);

	//From NodeObserver
	void notifyBeginNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&);
	void notifyEndNodeChange(const VNode*, const std::vector<ecf::Aspect::Type>&,const VNodeChange&) {}

	//From ServerObserver
	void notifyDefsChanged(ServerHandler* server,const std::vector<ecf::Aspect::Type>&);
	void notifyServerDelete(ServerHandler* server);
	void notifyBeginServerClear(ServerHandler* server) {};
	void notifyEndServerClear(ServerHandler* server) {};
	void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {};
	void notifyEndServerScan(ServerHandler* server) {};
	void notifyServerConnectState(ServerHandler* server);
	void notifyServerActivityChanged(ServerHandler* server);

	void rerender();

	void writeSettings(VSettings *vs);
	void readSettings(VSettings *vs);

public Q_SLOTS:
	void setPath(QString);
	void setPath(VInfo_ptr);

protected Q_SLOTS:
	void slotContextMenu(const QPoint&);
	void nodeItemSelected();
	void menuItemSelected();
	void slotRefreshServer();

Q_SIGNALS:
	void selected(VInfo_ptr);

protected:
	void clearLayout();
	void adjust(VInfo_ptr info,ServerHandler** serverOut,bool &sameServer);
	void loadMenu(const QPoint& pos,VInfo_ptr p);
	VInfo_ptr nodeAt(int);
	int findInPath(VInfo_ptr p1,VInfo_ptr p2,bool sameServer);
	void infoIndex(int idx);
	void paintEvent(QPaintEvent *);
	void reset();

	QList<NodePathNodeItem*> nodeItems_;
	QList<NodePathMenuItem*> menuItems_;

	QHBoxLayout* layout_;
	VInfo_ptr info_;
	VInfo_ptr infoFull_;
	int infoIndex_;
	QToolButton* reloadTb_;

	//When it is set to "true": if a node item (i.e. not a menu) is selected the breadcrumbs
	//will stay the same and only will change the current node/selection!
	bool stayInParent_;

	//When active_ is "true" the widget is visible and can receive and display data. When it
	//is "false" the widget is hidden and cannot hold any data.
	bool active_;
};

#endif
