//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWERUTIL_HPP
#define VIEWERUTIL_HPP

#include <QBrush>
#include <QCursor>
#include <QPixmap>
#include <QSettings>
#include <QString>

class QAction;
class QButtonGroup;
class QComboBox;
class QStackedWidget;
class QTabWidget;
class QTreeView;

class ViewerUtil
{
public:
   static void initComboBox(QSettings&,QString key,QComboBox* cb);
   static void initComboBoxByData(QString dataValue,QComboBox* cb);
   static bool initTreeColumnWidth(QSettings& settings,QString key,QTreeView *tree);
   static void saveTreeColumnWidth(QSettings& settings,QString key,QTreeView *tree);
   static void initStacked(QSettings& settings,QString key,QStackedWidget *stacked);
   static void initButtonGroup(QSettings& settings,QString key,QButtonGroup *bg);
   static void initCheckableAction(QSettings& settings,QString key,QAction *ac);
   static QBrush lineEditGreenBg();
   static QBrush lineEditRedBg();
   static QBrush lineEditBg(QColor col);
   static void toClipboard(QString txt);
   static QString fromClipboard();
   static void setOverrideCursor(QCursor cursor);
   static void restoreOverrideCursor();
};

#endif // VIEWERUTIL_HPP

