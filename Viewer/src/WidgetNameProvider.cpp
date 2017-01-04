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
#include <QTabWidget>
#include <QTabBar>
#include <QToolBar>

void WidgetNameProvider::nameButtons(QToolBar* tb)
{
    Q_FOREACH(QAction *ac,tb->actions())
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

void WidgetNameProvider::nameTabBar(QTabWidget* t)
{
    //if(t->tabBar()->objectName().isEmpty())
        t->tabBar()->setObjectName("bar");
}
