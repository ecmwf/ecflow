//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef MENUCONFIGDIALOG_HPP_
#define MENUCONFIGDIALOG_HPP_

#include <QDialog>
#include <QTreeWidget>
#include <QSplitter>

#include "MenuHandler.hpp"

class ConfigTreeWidget : public QTreeWidget
{
public:
  ConfigTreeWidget()
  {
  }

  explicit ConfigTreeWidget(QSplitter*s) : QTreeWidget(s)
  {
    resize(200, 300);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
/*
    QTreeWidgetItem* parentItem = new QTreeWidgetItem(this);
    parentItem->setText(0, "Test");
    parentItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);

    for(int i = 0; i < 10; ++i)
    {
      QTreeWidgetItem* pItem = new QTreeWidgetItem(parentItem);
      pItem->setText(0, QString("Number %1").arg(i) );
      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
      pItem->addChild(pItem);
    }*/
  };

/*private:
  virtual void  dropEvent(QDropEvent * event)
  {
    QModelIndex droppedIndex = indexAt( event->pos() );

    if( !droppedIndex.isValid() )
      return;

    QTreeWidget::dropEvent(event);
  }*/
};


#include "ui_MenuConfigDialog.h"

class MenuConfigDialog : public QDialog, private Ui::MenuConfigDialog
{
	Q_OBJECT

public:
	explicit MenuConfigDialog(QWidget *parent = 0);
	~MenuConfigDialog() {};

	void updateMenuTree(Menu *menu);

	void reject();


//public Q_SLOTS:
//	void insertCurrentText();


private:
	void addChildrenToMenuTree(Menu *menu, QTreeWidgetItem *parent);

};


#endif
