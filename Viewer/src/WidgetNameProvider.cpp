//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WidgetNameProvider.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QDialogButtonBox>
#include <QList>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBar>

void WidgetNameProvider::nameChildren(QWidget* w)
{
    nameButtons(w->actions());

    Q_FOREACH(QToolBar* tb,w->findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameButtons(tb->actions());
    }

    Q_FOREACH(QDialogButtonBox* bb,w->findChildren<QDialogButtonBox*>(QString()))
    {
        nameButtons(bb);
    }

    Q_FOREACH(QTabWidget* t,w->findChildren<QTabWidget*>(QString()))
    {
        nameTabWidget(t);
    }

    Q_FOREACH(QScrollArea* sa,w->findChildren<QScrollArea*>(QString()))
    {
        nameViewport(sa->viewport());
    }
}


void WidgetNameProvider::nameButtons(QList<QAction*> acLst)
{
    Q_FOREACH(QAction *ac,acLst)
    {
        Q_FOREACH(QWidget* w,ac->associatedWidgets())
        {
            if(w->objectName().isEmpty())
            {
                w->setObjectName(ac->objectName());
            }
        }
    }
}

void WidgetNameProvider::nameButtons(QDialogButtonBox* bb)
{
    Q_FOREACH(QAbstractButton* pb, bb->buttons())
    {
        if(pb->objectName().isEmpty())
        {
            QString t=pb->text();
            pb->setObjectName(t.remove("&"));
        }
    }
}

void WidgetNameProvider::nameTabWidget(QTabWidget* t)
{
    Q_FOREACH(QTabBar* tb,t->findChildren<QTabBar*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameTabBar(tb);
    }

    Q_FOREACH(QStackedWidget* tb,t->findChildren<QStackedWidget*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameStacked(tb);
    }
}

void WidgetNameProvider::nameTabBar(QTabBar* t)
{
    if(t->objectName().isEmpty() ||
       t->objectName() == "qt_tabwidget_tabbar")
    {
        t->setObjectName("bar");
    }
}

void WidgetNameProvider::nameStacked(QStackedWidget* t)
{
    if(t->objectName().isEmpty() ||
       t->objectName() == "qt_tabwidget_stackedwidget")
    {
        t->setObjectName("stacked");
    }
}

void WidgetNameProvider::nameViewport(QWidget* t)
{
    if(!t)
        return;

    if(t->objectName().isEmpty() ||
       t->objectName() == "qt_scrollarea_viewport")
    {
        t->setObjectName("viewport");
    }
}
