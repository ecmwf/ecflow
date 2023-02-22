//============================================================================
// Copyright 2009- ECMWF.
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
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLabel>
#include <QLinearGradient>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeView>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

void ViewerUtil::initComboBox(QSettings& settings, QString key, QComboBox* cb) {
    Q_ASSERT(cb);
    QString txt = settings.value(key).toString();
    for (int i = 0; i < cb->count(); i++) {
        if (cb->itemText(i) == txt) {
            cb->setCurrentIndex(i);
            return;
        }
    }

    if (cb->currentIndex() == -1)
        cb->setCurrentIndex(0);
}

void ViewerUtil::initComboBoxByData(QString dataValue, QComboBox* cb) {
    Q_ASSERT(cb);
    for (int i = 0; i < cb->count(); i++) {
        if (cb->itemData(i).toString() == dataValue) {
            cb->setCurrentIndex(i);
            return;
        }
    }

    if (cb->currentIndex() == -1)
        cb->setCurrentIndex(0);
}

bool ViewerUtil::initTreeColumnWidth(QSettings& settings, QString key, QTreeView* tree) {
    Q_ASSERT(tree);

    QStringList dataColumns = settings.value(key).toStringList();
    for (int i = 0; i < tree->model()->columnCount() - 1 && i < dataColumns.size(); i++) {
        tree->setColumnWidth(i, dataColumns[i].toInt());
    }

    return (dataColumns.size() >= tree->model()->columnCount() - 1);
}

void ViewerUtil::saveTreeColumnWidth(QSettings& settings, QString key, QTreeView* tree) {
    QStringList dataColumns;
    for (int i = 0; i < tree->model()->columnCount() - 1; i++) {
        dataColumns << QString::number(tree->columnWidth(i));
    }
    settings.setValue(key, dataColumns);
}

void ViewerUtil::initStacked(QSettings& settings, QString key, QStackedWidget* stacked) {
    Q_ASSERT(stacked);

    int v = settings.value(key).toInt();
    if (v >= 0 && v < stacked->count())
        stacked->setCurrentIndex(v);
}

void ViewerUtil::initButtonGroup(QSettings& settings, QString key, QButtonGroup* bg) {
    Q_ASSERT(bg);

    int v = settings.value(key).toInt();
    if (v >= 0 && v < bg->buttons().count()) {
        bg->buttons().at(v)->setChecked(true);
        bg->buttons().at(v)->click();
    }
}

void ViewerUtil::initCheckableAction(QSettings& settings, QString key, QAction* ac) {
    Q_ASSERT(ac);

    if (settings.contains(key)) {
        ac->setChecked(settings.value(key).toBool());
    }
}

QBrush ViewerUtil::lineEditGreenBg() {
    // return lineEditBg(QColor(189,239,205));
    return lineEditBg(QColor(210, 255, 224));
}

QBrush ViewerUtil::lineEditRedBg() {
    return lineEditBg(QColor(234, 215, 214));
}

QBrush ViewerUtil::lineEditBg(QColor col) {
    QLinearGradient grad;
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad.setStart(0, 0);
    grad.setFinalStop(0, 1);

    grad.setColorAt(0, col);
    grad.setColorAt(0.1, col.lighter(105));
    grad.setColorAt(0.1, col.lighter(108));
    grad.setColorAt(0.9, col.lighter(108));
    grad.setColorAt(0.9, col.lighter(105));
    grad.setColorAt(1, col);
    return QBrush(grad);
}

void ViewerUtil::toClipboard(QString txt) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QClipboard* cb = QGuiApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#else
    QClipboard* cb = QApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#endif
}

QString ViewerUtil::fromClipboard() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QGuiApplication::clipboard()->text();
#else
    return QApplication::clipboard()->text();
#endif
}

void ViewerUtil::setOverrideCursor(QCursor cursor) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(cursor);
#else
    QApplication::setOverrideCursor(cursor);
#endif
}

void ViewerUtil::restoreOverrideCursor() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::restoreOverrideCursor();
#else
    QApplication::restoreOverrideCursor();
#endif
}

QString ViewerUtil::formatDuration(unsigned int delta) // in seconds
{
    int day  = static_cast<int>(delta / 86400);
    int hour = (static_cast<int>(delta % 86400) / 3600);
    int min  = (static_cast<int>(delta % 3600) / 60);
    int sec  = static_cast<int>(delta % 60);

    QString s;

    if (day > 0) {
        s += QString::number(day) + "d ";
    }

    if (hour > 0) {
        s += QString::number(hour) + "h ";
    }

    if (min > 0) {
        s += QString::number(min) + "m ";
    }

    if (sec > 0 || s.isEmpty()) {
        s += QString::number(sec) + "s";
    }

    return s;
}

int ViewerUtil::textWidth(const QFontMetrics& fm, QString txt, int len) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return fm.horizontalAdvance(txt, len);
#else
    return fm.width(txt, len);
#endif
}

int ViewerUtil::textWidth(const QFontMetrics& fm, QChar ch) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return fm.horizontalAdvance(ch);
#else
    return fm.width(ch);
#endif
}

// Taken from the qt-everywhere-opensource-4.8.3 source code:
// src/corelib/tools/qregexp.cpp:wc2rx()
// See JIRA issue ECFLOW-1753.
QString ViewerUtil::wildcardToRegex(const QString& wc_str) {
    const bool enableEscaping = true;
    const int wclen           = wc_str.length();
    QString rx;
    int i           = 0;
    bool isEscaping = false; // the previous character is '\'
    const QChar* wc = wc_str.unicode();

    while (i < wclen) {
        const QChar c = wc[i++];
        switch (c.unicode()) {
            case '\\':
                if (enableEscaping) {
                    if (isEscaping) {
                        rx += QLatin1String("\\\\");
                    }                 // we insert the \\ later if necessary
                    if (i == wclen) { // the end
                        rx += QLatin1String("\\\\");
                    }
                }
                else {
                    rx += QLatin1String("\\\\");
                }
                isEscaping = true;
                break;
            case '*':
                if (isEscaping) {
                    rx += QLatin1String("\\*");
                    isEscaping = false;
                }
                else {
                    rx += QLatin1String(".*");
                }
                break;
            case '?':
                if (isEscaping) {
                    rx += QLatin1String("\\?");
                    isEscaping = false;
                }
                else {
                    rx += QLatin1Char('.');
                }

                break;
            case '$':
            case '(':
            case ')':
            case '+':
            case '.':
            case '^':
            case '{':
            case '|':
            case '}':
                if (isEscaping) {
                    isEscaping = false;
                    rx += QLatin1String("\\\\");
                }
                rx += QLatin1Char('\\');
                rx += c;
                break;
            case '[':
                if (isEscaping) {
                    isEscaping = false;
                    rx += QLatin1String("\\[");
                }
                else {
                    rx += c;
                    if (wc[i] == QLatin1Char('^'))
                        rx += wc[i++];
                    if (i < wclen) {
                        if (rx[i] == QLatin1Char(']'))
                            rx += wc[i++];
                        while (i < wclen && wc[i] != QLatin1Char(']')) {
                            if (wc[i] == QLatin1Char('\\'))
                                rx += QLatin1Char('\\');
                            rx += wc[i++];
                        }
                    }
                }
                break;

            case ']':
                if (isEscaping) {
                    isEscaping = false;
                    rx += QLatin1String("\\");
                }
                rx += c;
                break;

            default:
                if (isEscaping) {
                    isEscaping = false;
                    rx += QLatin1String("\\\\");
                }
                rx += c;
        }
    }
    return rx;
}

QFont ViewerUtil::findMonospaceFont() {
    QStringList lst{"Monospace", "Courier New", "Menlo", "Courier", "Monaco"};

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto fLst = QFontDatabase::families();
#else
    QFontDatabase db = QFontDatabase();
    auto fLst        = db.families();
#endif
    for (auto s : lst) {
        for (auto fMem : fLst) {
            if (fMem == s || fMem.startsWith(s + "[")) {
                QFont f(fMem);
                f.setFixedPitch(true);
                f.setPointSize(10);
                return f;
            }
        }
    }

    QFont fr;
    fr.setPointSize(10);
    return fr;
}

QIcon ViewerUtil::makeExpandIcon(bool targetOnRight) {
    QIcon ic;
    ic.addPixmap(QPixmap(":/viewer/expand_left.svg"), QIcon::Normal, (targetOnRight ? QIcon::On : QIcon::Off));
    ic.addPixmap(QPixmap(":/viewer/expand_right.svg"), QIcon::Normal, (targetOnRight ? QIcon::Off : QIcon::On));
    return ic;
}

void ViewerUtil::showShortcutInContextMenu(QAction* ac) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    ac->setShortcutVisibleInContextMenu(true);
#endif
}
