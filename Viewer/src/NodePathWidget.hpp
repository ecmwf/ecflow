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

#include "NodeObserver.hpp"
#include "ServerObserver.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

#include <QLinearGradient>
#include <QWidget>

#include <string>

class QAction;
class QHBoxLayout;
class QMenu;
class QToolButton;

class Node;
class NodePathWidget;
class PropertyMapper;
class VProperty;
class VSettings;

class NodePathItem;

class BcWidget : public QWidget, public VPropertyObserver
{
    
Q_OBJECT

public:
    explicit BcWidget(QWidget *parent=0);
    ~BcWidget();
    
    void reset(QList<NodePathItem*>);
    void reset(int idx,QString text,QColor bgCol,QColor fontCol);
    void clear();
    
    void notifyChange(VProperty*);

Q_SIGNALS:
    void itemSelected(int);
    void menuSelected(int,QPoint);
    
protected:
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent* event);
    void changeEvent(QEvent* event);
    
    void updateSettings();
    void reset(int);
    void crePixmap();
    void updatePixmap(int);
    
    QFont font_;
    QPixmap pix_;
    int hPadding_;
    int vPadding_;
    int hMargin_;
    int vMargin_;
    int triLen_;
    int gap_;
    int width_;
    int height_;
    int itemHeight_;
    QString emptyText_;
    QRect emptyRect_;
    QList<NodePathItem*> items_; 
    
    PropertyMapper* prop_;
    bool useGrad_;
    int gradLighter_;
};


class NodePathItem 
{

friend class BcWidget;    
    
public:
    NodePathItem(int index,QString text,QColor bgCol,QColor fontCol,bool hasMenu,bool current);
    void setCurrent(bool);
    void draw(QPainter*,bool,int);
    
protected:
    int index_;
    QString text_;
    QColor bgCol_;
    QColor borderCol_;
    QColor fontCol_;
    QPolygon shape_;
    QRect textRect_;
    bool current_;
    bool hasMenu_;
    QLinearGradient grad_;
    bool enabled_;
    static QColor disabledBgCol_;
    static QColor disabledBorderCol_;
    static QColor disabledFontCol_;
};


class NodePathWidget : public QWidget, public NodeObserver, public ServerObserver, public VInfoObserver
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
    void notifyBeginServerClear(ServerHandler* server);
    void notifyEndServerClear(ServerHandler* server) {}
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
    void notifyEndServerScan(ServerHandler* server);
	void notifyServerConnectState(ServerHandler* server);
	void notifyServerActivityChanged(ServerHandler* server);

    //From VInfoObserver
    void notifyDelete(VInfo*) {}
    void notifyDataLost(VInfo*);

	void rerender();

	void writeSettings(VSettings *vs);
	void readSettings(VSettings *vs);

public Q_SLOTS:
	void setPath(QString);
	void setPath(VInfo_ptr);

protected Q_SLOTS:
	void slotContextMenu(const QPoint&);
	void slotNodeSelected(int);
	void slotMenuSelected(int,QPoint);
	void slotRefreshServer();

Q_SIGNALS:
	void selected(VInfo_ptr);

protected:
	void clearItems();
	void adjust(VInfo_ptr info,ServerHandler** serverOut,bool &sameServer);
	void loadMenu(const QPoint& pos,VInfo_ptr p);
	VInfo_ptr nodeAt(int);
	void infoIndex(int idx);
	void paintEvent(QPaintEvent *);
	void reset();
    void updateSettings();

	QList<NodePathItem*> nodeItems_;
	
	QHBoxLayout* layout_;
	VInfo_ptr info_;
	VInfo_ptr infoFull_;
	QToolButton* reloadTb_;
    BcWidget* bc_;
   
   
	//When active_ is "true" the widget is visible and can receive and display data. When it
	//is "false" the widget is hidden and cannot hold any data.
	bool active_;
};

#endif
