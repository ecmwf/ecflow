/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TextFormat_HPP
#define ecflow_viewer_TextFormat_HPP

#include <QColor>
#include <QList>
#include <QString>

class QAction;

namespace Viewer {
QString formatShortCut(QString);
QString formatShortCut(QAction*);
void addShortCutToToolTip(QList<QAction*>);
QString formatBoldText(QString, QColor);
QString formatItalicText(QString, QColor);
QString formatText(QString, QColor);
QString formatTableThText(QString txt, QColor col);
QString formatTableTrBg(QString txt, QColor col);
QString formatTableTrText(QString txt);
QString formatTableTdText(QString txt, QColor col);
QString formatTableTdText(QString txt);
QString formatTableTdBg(QString txt, QColor col);
QString formatTableRow(QString col1Text, QString col2Text, QColor bg, QColor fg, bool boldCol1);
QString formatTableRow(QString col1Text, QString col2Text, bool boldCol1);

} // namespace Viewer

#endif /* ecflow_viewer_TextFormat_HPP  */
