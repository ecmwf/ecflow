//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <QCloseEvent>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>

#include "AddLabelDialog.hpp"
#include "CommandHandler.hpp"
#include "EditorInfoLabel.hpp"
#include "SessionHandler.hpp"
#include "VAttribute.hpp"

#include "ui_AddLabelDialog.h"

AddLabelDialog::AddLabelDialog(VInfo_ptr info, QWidget* parent) :
    QDialog(parent),
    info_(info)
{
    if(!info_ || !info_->isNode() || !info_->node())
    {
        QDialog::reject();
        return;
    }

    ui_->setupUi(this);

    QLayoutItem *item;
    item=ui_->grid->itemAtPosition(1,0);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft|Qt::AlignTop);

    Q_ASSERT(info_);
    ui_->header->setInfo(QString::fromStdString(info_->path()),"Label");

    connect(ui_->buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));
    connect(ui_->buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));

    readSettings();
}

void AddLabelDialog::accept()
{
    std::string val=ui_->valueTe->toPlainText().toStdString();
    std::string name=ui_->nameLe->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"add","label", name, val);
    CommandHandler::run(info_,cmd);

    writeSettings();
    QDialog::accept();
}

void AddLabelDialog::reject()
{
    writeSettings();
    QDialog::accept();
}

void AddLabelDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    writeSettings();
}

void AddLabelDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("AddLabelDialog")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void AddLabelDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("AddLabelDialog")),
                       QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(310,200));
    }

    settings.endGroup();
}
