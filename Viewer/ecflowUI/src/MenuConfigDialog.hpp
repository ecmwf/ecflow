/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_MenyConfigDialog_HPP
#define ecflow_viewer_MenyConfigDialog_HPP

#include <QDialog>
#include <QSplitter>
#include <QTreeWidget>

#include "MenuHandler.hpp"

class ConfigTreeWidget : public QTreeWidget {
public:
    ConfigTreeWidget() = default;

    explicit ConfigTreeWidget(QSplitter* s)
        : QTreeWidget(s) {
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

class MenuConfigDialog : public QDialog, private Ui::MenuConfigDialog {
    Q_OBJECT

public:
    explicit MenuConfigDialog(QWidget* parent = nullptr);
    ~MenuConfigDialog() override = default;

    void updateMenuTree(Menu* menu);

    void reject() override;

    // public Q_SLOTS:
    //	void insertCurrentText();

private:
    void addChildrenToMenuTree(Menu* menu, QTreeWidgetItem* parent);
};

#endif /* ecflow_viewer_MenyConfigDialog_HPP */
