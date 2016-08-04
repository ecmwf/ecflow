//============================================================================
// Copyright 2016 ECMWF.
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
class NodePathEllipsisItem;

class BcWidget : public QWidget, public VPropertyObserver
{
    
Q_OBJECT

public:
    explicit BcWidget(QWidget *parent=0);
    ~BcWidget();
    
    void reset(QList<NodePathItem*>,int);
    void reset(int idx,QString text,QColor bgCol,QColor fontCol);
    void clear();
    void adjustSize(int);
    
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
    void resetBorder(int);
    void crePixmap();
    void updatePixmap(int);
    bool isFull() const;
    int estimateWidth(int,int,int);
    int adjustItems(int,int,int,int);
    int adjustVisibleItems(int,int,int,int);

    QFont font_;
    QPixmap pix_;   
    int hMargin_;
    int vMargin_;   
    int gap_;
    int width_;
    int maxWidth_;
    int height_;
    int itemHeight_;
    QString emptyText_;
    QRect emptyRect_;
    QList<NodePathItem*> items_;
    NodePathEllipsisItem* ellipsisItem_;
    
    PropertyMapper* prop_;
    bool useGrad_;
    int gradLighter_;
    int hovered_;
    bool elided_;
};

class NodePathItem 
{

friend class BcWidget;    
    
public:
    NodePathItem(int index,QString text,QColor bgCol,QColor fontCol,bool hasMenu,bool current);
    void setCurrent(bool);
    virtual void draw(QPainter*,bool,int);
    int adjust(int xp,int yp, int elidedLen=0);
    int estimateRightPos(int xp,int elidedLen=0);
    void resetBorder(bool hovered);
    void reset(QString text,QColor bgCol,QColor fontCol,bool hovered);
    int textLen() const;
    static int height();

protected:
    virtual void makeShape(int xp,int yp,int len);
    virtual int rightPos(int xp,int len) const;

    int index_;
    QString text_;
    QString elidedText_;
    QColor bgCol_;
    QColor borderCol_;
    QColor fontCol_;
    QPolygon shape_;
    QRect textRect_;

    bool current_;
    bool hasMenu_;
    bool visible_;
    QLinearGradient grad_;
    bool enabled_;
    static QColor disabledBgCol_;
    static QColor disabledBorderCol_;
    static QColor disabledFontCol_;

    static int triLen_;
    static int height_;
    static int hPadding_;
    static int vPadding_;
};

class NodePathServerItem : public NodePathItem
{

friend class BcWidget;

public:
    NodePathServerItem(int index,QString text,QColor bgCol,QColor fontCol,bool hasMenu,bool current) :
         NodePathItem(index,text,bgCol,fontCol,hasMenu,current) {}
protected:
    void makeShape(int xp,int yp,int len);
    int rightPos(int xp,int len) const;
};


class NodePathEllipsisItem : public NodePathItem
{

friend class BcWidget;

public:
    NodePathEllipsisItem();
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
    void resizeEvent(QResizeEvent *);
	void reset();
    void updateSettings();
    int bcWidth();

	QList<NodePathItem*> nodeItems_;
	
	QHBoxLayout* layout_;
	VInfo_ptr info_;
	VInfo_ptr infoFull_;
#if 0
    QToolButton* reloadTb_;
#endif
    BcWidget* bc_;
   
   
	//When active_ is "true" the widget is visible and can receive and display data. When it
	//is "false" the widget is hidden and cannot hold any data.
	bool active_;
};

#endif
