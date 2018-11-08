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

#include <QtGlobal>
#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAction>
#include <QButtonGroup>
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QLinearGradient>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeView>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#else
#include <QApplication>
#endif

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

void ViewerUtil::initComboBoxByData(QString dataValue,QComboBox* cb)
{
    Q_ASSERT(cb);
    for(int i=0; i < cb->count(); i++)
    {
        if(cb->itemData(i).toString() == dataValue)
        {
            cb->setCurrentIndex(i);
            return;
        }
    }

    if(cb->currentIndex() == -1)
        cb->setCurrentIndex(0);
}

bool ViewerUtil::initTreeColumnWidth(QSettings& settings,QString key,QTreeView *tree)
{
    Q_ASSERT(tree);

    QStringList dataColumns=settings.value(key).toStringList();
    for(int i=0; i < tree->model()->columnCount()-1 && i < dataColumns.size(); i++)
    {
        tree->setColumnWidth(i,dataColumns[i].toInt());
    }

    return (dataColumns.size() >= tree->model()->columnCount()-1);
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

QBrush ViewerUtil::lineEditGreenBg()
{
    //return lineEditBg(QColor(189,239,205));
    return lineEditBg(QColor(210,255,224));
}

QBrush ViewerUtil::lineEditRedBg()
{
    return lineEditBg(QColor(234,215,214));
}

QBrush ViewerUtil::lineEditBg(QColor col)
{
    QLinearGradient grad;
    grad.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad.setStart(0,0);
    grad.setFinalStop(0,1);

    grad.setColorAt(0,col);
    grad.setColorAt(0.1,col.lighter(105));
    grad.setColorAt(0.1,col.lighter(108));
    grad.setColorAt(0.9,col.lighter(108));
    grad.setColorAt(0.9,col.lighter(105));
    grad.setColorAt(1,col);
    return QBrush(grad);
}

void ViewerUtil::toClipboard(QString txt)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QClipboard* cb=QGuiApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#else
    QClipboard* cb=QApplication::clipboard();
    cb->setText(txt, QClipboard::Clipboard);
    cb->setText(txt, QClipboard::Selection);
#endif
}

QString ViewerUtil::fromClipboard()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QGuiApplication::clipboard()->text();
#else
    return QApplication::clipboard()->text();
#endif
}

void ViewerUtil::setOverrideCursor(QCursor cursor)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(cursor);
#else
    QApplication::setOverrideCursor(cursor);
#endif
}

void ViewerUtil::restoreOverrideCursor()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#else
    QApplication::restoreOverrideCursor();
#endif
}
