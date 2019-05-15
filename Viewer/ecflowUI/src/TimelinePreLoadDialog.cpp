//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelinePreLoadDialog.hpp"

#include <QtGlobal>
#include <QFileInfo>
#include <QButtonGroup>
#include <QSettings>
#include <QTreeWidgetItem>

#include "TimelineData.hpp"
#include "TimelineFileList.hpp"
#include "SessionHandler.hpp"

#include "ui_TimelinePreLoadDialog.h"

TimelinePreLoadDialog::TimelinePreLoadDialog(QWidget *parent) :
    ui_(new Ui::TimelinePreLoadDialog)
{
    ui_->setupUi(this);

    //the loadable files
    ui_->label->setText(tr("List of logs to be <b>loaded</b>:"));

    ui_->tree->setRootIsDecorated(false);
    ui_->tree->setAllColumnsShowFocus(true);
    ui_->tree->setColumnCount(4);
    QStringList cols;
    cols << "File" << "Start date" << "End date" << "File size";
    ui_->tree->setHeaderLabels(cols);
   // ui_->tree->setStyleSheet("QTreeWidget{background: rgb(234,245,227);}");

    //the wrong files
    ui_->labelBad->setText(tr("List of the files <b>cannot be</b> loaded:"));

    ui_->treeBad->setRootIsDecorated(false);
    ui_->treeBad->setAllColumnsShowFocus(true);
    ui_->treeBad->setColumnCount(3);
    cols.clear();
    cols << "File" << "File size" << "Error message";
    ui_->treeBad->setHeaderLabels(cols);
    ui_->treeBad->setStyleSheet("QTreeWidget{color: rgb(222,15,32);}");

    readSettings();
}

TimelinePreLoadDialog::~TimelinePreLoadDialog()
{
    writeSettings();
}

void TimelinePreLoadDialog::init(const TimelineFileList& lst)
{
    bool hasBadFile=false;
    for(int i=0; i < lst.items().count(); i++)
    {
        if(lst.items()[i].loadable_)
        {
            QTreeWidgetItem* item=new QTreeWidgetItem(ui_->tree);
            item->setData(0,Qt::DisplayRole,lst.items()[i].fileName_);
            item->setData(1,Qt::DisplayRole,
                       TimelineItem::toQDateTime(lst.items()[i].startTime_).toString((" hh:mm:ss dd-MMM-yyyy")));
            item->setData(2,Qt::DisplayRole,
                      TimelineItem::toQDateTime(lst.items()[i].endTime_).toString((" hh:mm:ss dd-MMM-yyyy")));
            item->setData(3,Qt::DisplayRole,
                          QString::number(lst.items()[i].size_/(1024*1024)) + " MB");

        }
        else
        {
            hasBadFile=true;
            QTreeWidgetItem* item=new QTreeWidgetItem(ui_->treeBad);
            item->setData(0,Qt::DisplayRole,lst.items()[i].fileName_);
            item->setData(1,Qt::DisplayRole,
                          QString::number(lst.items()[i].size_/(1024*1024)) + " MB");
            item->setData(2,Qt::DisplayRole,lst.items()[i].message_);
        }
    }

    ui_->tree->resizeColumnToContents(1);
    ui_->tree->resizeColumnToContents(2);
    ui_->tree->resizeColumnToContents(3);

    if(hasBadFile)
    {
        ui_->treeBad->resizeColumnToContents(0);
    }
    else
    {
        ui_->labelBad->hide();
        ui_->treeBad->hide();
    }
}

void TimelinePreLoadDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TimelinePreLoadDialog")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());

    settings.endGroup();
}

void TimelinePreLoadDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TimelinePreLoadDialog")),
                       QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(500,280));
    }

    settings.endGroup();
}
