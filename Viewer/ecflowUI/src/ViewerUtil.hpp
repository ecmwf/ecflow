/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ViewerUtil_HPP
#define ecflow_viewer_ViewerUtil_HPP

#include <QBrush>
#include <QCursor>
#include <QIcon>
#include <QPixmap>
#include <QSettings>
#include <QString>

#ifdef ECFLOW_LOGVIEW
    #include <QtCharts>
#endif

class QAction;
class QButtonGroup;
class QComboBox;
class QStackedWidget;
class QTabWidget;
class QTreeView;
class QFontMetrics;

class ViewerUtil {
public:
    static void initComboBox(QSettings&, QString key, QComboBox* cb);
    static void initComboBoxByData(QString dataValue, QComboBox* cb);
    static bool initTreeColumnWidth(QSettings& settings, QString key, QTreeView* tree);
    static void saveTreeColumnWidth(QSettings& settings, QString key, QTreeView* tree);
    static void initStacked(QSettings& settings, QString key, QStackedWidget* stacked);
    static void initButtonGroup(QSettings& settings, QString key, QButtonGroup* bg);
    static void initCheckableAction(QSettings& settings, QString key, QAction* ac);
    static QBrush lineEditGreenBg();
    static QBrush lineEditRedBg();
    static QBrush lineEditBg(QColor col);
    static void toClipboard(QString txt);
    static QString fromClipboard();
    static void setOverrideCursor(QCursor cursor);
    static void restoreOverrideCursor();
    static QString formatDuration(unsigned int);
    static int textWidth(const QFontMetrics& fm, QString txt, int len = -1);
    static int textWidth(const QFontMetrics& fm, QChar ch);
    static QString wildcardToRegex(const QString&);
    static QFont findMonospaceFont();
    static QIcon makeExpandIcon(bool targetOnRight);
    static void showShortcutInContextMenu(QAction*);
#ifdef ECFLOW_LOGVIEW
    static QAbstractAxis* chartAxisX(QChart* chart);
    static QAbstractAxis* chartAxisY(QChart* chart);
#endif
};

#endif /* ecflow_viewer_ViewerUtil_HPP */
