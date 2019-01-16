//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WidgetNameProvider.hpp"

#include <QtGlobal>
#include <QAbstractButton>
#include <QAbstractScrollArea>
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_FOREACH(QToolBar* tb,w->findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameButtons(tb->actions());
    }
#else
    Q_FOREACH(QToolBar* tb,w->findChildren<QToolBar*>(QString()))
    {
        if(tb->parent() == w)
            nameButtons(tb->actions());
    }
#endif

    Q_FOREACH(QDialogButtonBox* bb,w->findChildren<QDialogButtonBox*>(QString()))
    {
        nameButtons(bb);
    }

    Q_FOREACH(QTabWidget* t,w->findChildren<QTabWidget*>(QString()))
    {
        nameTabWidget(t);
    }

    Q_FOREACH(QAbstractScrollArea* sa,w->findChildren<QAbstractScrollArea*>(QString()))
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_FOREACH(QTabBar* tb,t->findChildren<QTabBar*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameTabBar(tb);
    }
#else
    Q_FOREACH(QTabBar* tb,t->findChildren<QTabBar*>(QString()))
    {
        if(tb->parent() == t)
            nameTabBar(tb);
    }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_FOREACH(QStackedWidget* tb,t->findChildren<QStackedWidget*>(QString(),Qt::FindDirectChildrenOnly))
    {
        nameStacked(tb);
    }
#else
    Q_FOREACH(QStackedWidget* tb,t->findChildren<QStackedWidget*>(QString()))
    {
        if(tb->parent() == t)
            nameStacked(tb);
    }
#endif
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
