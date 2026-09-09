/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_WidgetNameProvider_HPP
#define ecflow_viewer_WidgetNameProvider_HPP

#include <QList>

class QAction;
class QDialogButtonBox;
class QStackedWidget;
class QToolBar;
class QTabBar;
class QTabWidget;
class QWidget;

class WidgetNameProvider {
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

#endif /* ecflow_viewer_WidgetNameProvider_HPP */
