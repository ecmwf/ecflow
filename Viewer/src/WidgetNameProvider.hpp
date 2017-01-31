//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef WIDGETNAME_HPP
#define WIDGETNAME_HPP

#include <QList>

class QAction;
class QDialogButtonBox;
class QStackedWidget;
class QToolBar;
class QTabBar;
class QTabWidget;
class QWidget;

class WidgetNameProvider
{
public:
    static void nameChildren(QWidget* w);

private:
    static void nameButtons(QList<QAction*>);
    static void nameButtons(QDialogButtonBox* bb);
    static void nameTabWidget(QTabWidget* t);
    static void nameTabBar(QTabBar* t);
    static void nameStacked(QStackedWidget* t);
    static void nameViewport(QWidget* t);
};

#endif // WIDGETNAME_HPP

