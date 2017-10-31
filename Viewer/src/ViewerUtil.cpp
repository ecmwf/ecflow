//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ViewerUtil.hpp"

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeView>

void ViewerUtil::initComboBox(QSettings& settings,QString key,QComboBox* cb)
{
    Q_ASSERT(cb);
    QString txt=settings.value(key).toString();
    for(int i=0; i < cb->count(); i++)
    {
        if(cb->itemText(i) == txt)
        {
            cb->setCurrentIndex(i);
            return;
        }
    }

    if(cb->currentIndex() == -1)
        cb->setCurrentIndex(0);
}


void ViewerUtil::initTreeColumnWidth(QSettings& settings,QString key,QTreeView *tree)
{
    Q_ASSERT(tree);

    QStringList dataColumns=settings.value(key).toStringList();
    for(int i=0; i < tree->model()->columnCount()-1 && i < dataColumns.size(); i++)
    {
        tree->setColumnWidth(i,dataColumns[i].toInt());
    }
}

void ViewerUtil::saveTreeColumnWidth(QSettings& settings,QString key,QTreeView *tree)
{
    QStringList dataColumns;
    for(int i=0; i < tree->model()->columnCount()-1; i++)
    {
        dataColumns << QString::number(tree->columnWidth(i));
    }
    settings.setValue(key,dataColumns);
}


void ViewerUtil::initStacked(QSettings& settings,QString key,QStackedWidget *stacked)
{
    Q_ASSERT(stacked);

    int v=settings.value(key).toInt();
    if(v >= 0 && v < stacked->count())
        stacked->setCurrentIndex(v);
}

void ViewerUtil::initButtonGroup(QSettings& settings,QString key,QButtonGroup *bg)
{
    Q_ASSERT(bg);

    int v=settings.value(key).toInt();
    if(v >= 0 && v < bg->buttons().count())
    {
        bg->buttons().at(v)->setChecked(true);
        bg->buttons().at(v)->click();
    }
}

void ViewerUtil::initCheckableAction(QSettings& settings,QString key,QAction *ac)
{
    Q_ASSERT(ac);

    if(settings.contains(key))
    {
        ac->setChecked(settings.value(key).toBool());
    }
}
