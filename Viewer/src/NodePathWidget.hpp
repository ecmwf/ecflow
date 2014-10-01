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

#include <QWidget>

#include "ViewNodeInfo.hpp"

#include <string>

class QAction;
class QHBoxLayout;
class QMenu;
class QSignalMapper;
class QToolButton;

class Node;

class NodePathWidgetItem
{
public:
    NodePathWidgetItem(QString n,QString fn) : name_(n), path_(fn), menuTb_(0), nameTb_(0), menu_(0) {}
	~NodePathWidgetItem();

  	QString name_;
	QString path_;
  	QToolButton* menuTb_;
  	QToolButton* nameTb_;
  	QMenu* menu_;
  	QRect rect_;
  	QColor colour_;
};

class NodePathWidget : public QWidget
{
Q_OBJECT

public:
	NodePathWidget(QWidget* parent=0);
	~NodePathWidget() {}

	void setPath(QString);
	void setPath(ViewNodeInfo_ptr);
	void setReloadAction(QAction*);

protected slots:
    void slotChangeNode(int);
	void slotChangeNode(QAction *);
	void slotContextMenu(const QPoint&);
	void slotShowNodeChildrenMenu();
	void slotEditable(bool);

signals:
  	void nodeClicked(int);
	void nodeSelected(ViewNodeInfo_ptr);
	void commandRequested(QString,QString);

protected:
	void clearLayout();
	QMenu* createNodeChildrenMenu(int,Node*,QWidget *);
	QString getPath(QToolButton*);
	QString getPath(QAction*);
	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void updateGr();

	ViewNodeInfo_ptr nodeAt(int);
	int nodeIndex(QPoint pos);

	QHBoxLayout *layout_;
	QSignalMapper* smp_;
	QList<NodePathWidgetItem*> items_;
	QAction* actionReload_;
	QToolButton* reloadTb_;
	QString path_;
	bool editable_;
	ViewNodeInfo_ptr info_;
	int height_;
	int paddingX_;
	int paddingY_;
	int textPaddingX_;
	int textPaddingY_;
	int gapX_;


	//static QList<MvQContextItem*> cmTbItems_;
	//static QList<MvQContextItem*> cmMenuItems_;

};

#endif
