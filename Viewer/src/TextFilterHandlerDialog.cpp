//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextFilterHandlerDialog.hpp"

#include "TextFilterHandler.hpp"
#include "SessionHandler.hpp"

#include <QDebug>
#include <QSettings>
#include <QTableWidgetItem>

TextFilterHandlerDialog::TextFilterHandlerDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    table_->addAction(actionEdit_);
    table_->addAction(actionDuplicate_);
    table_->addAction(actionRemove_);

    //Add actions for the toolbuttons
    editPb_->setDefaultAction(actionEdit_);
    removePb_->setDefaultAction(actionRemove_);
    duplicatePb_->setDefaultAction(actionDuplicate_);

    //Init the table
    reloadTable();

    //init
    readSettings();
}

TextFilterHandlerDialog::~TextFilterHandlerDialog()
{
    writeSettings();
}

void TextFilterHandlerDialog::reloadTable()
{
    //The itemlist is fairly small. For simplicity we do not use a model/view solution here just
    //a tablewidget. We reload the whole table widget whenerver there is a change in the list.

    table_->clear(); //it removes the headers as well

    QStringList headers;
    headers << "Name" << "Filter expression (regexp)";
    table_->setHorizontalHeaderLabels(headers);

    const std::vector<TextFilterItem>& items=TextFilterHandler::Instance()->items();
    table_->setRowCount(items.size());
    for(size_t i=0; i < items.size(); i++)
    {
        QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(items[i].name()));
        QTableWidgetItem *filterItem = new QTableWidgetItem(QString::fromStdString(items[i].filter()));
        table_->setItem(i, 0, nameItem);
        table_->setItem(i, 1, filterItem);
    }
}


void TextFilterHandlerDialog::on_actionEdit__triggered()
{
    //QModelIndex index=serverView->currentIndex();
    //editItem(index);
}

void TextFilterHandlerDialog::on_actionDuplicate__triggered()
{
    //QModelIndex index=serverView->currentIndex();
    //editItem(index);
}

void TextFilterHandlerDialog::on_actionRemove__triggered()
{
    int r=table_->currentRow();
    if(r >= 0)
    {
        TextFilterHandler::Instance()->remove(r);
        reloadTable();
    }
}

void TextFilterHandlerDialog::setItemToAdd(QString name,QString filter)
{
    addNameLe_->setText(name);
    addFilterLe_->setText(filter);
}

void TextFilterHandlerDialog::on_addPb__clicked()
{
    std::string name=addNameLe_->text().toStdString();
    std::string filter=addFilterLe_->text().toStdString();

    if(TextFilterHandler::Instance()->contains(name,filter))
    {
        return;
    }

    TextFilterHandler::Instance()->add(name,filter);
    reloadTable();
}

QString TextFilterHandlerDialog::settingsFile()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    return QString::fromStdString(cs->qtSettingsFile("TextFilterHandlerDialog"));
}

void TextFilterHandlerDialog::writeSettings()
{
    Q_ASSERT(settingsFile().isEmpty() == false);
    QSettings settings(settingsFile(),QSettings::NativeFormat);

    //We have to clear it not to remember all the previous windows
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void TextFilterHandlerDialog::readSettings()
{
    Q_ASSERT(settingsFile().isEmpty() == false);
    QSettings settings(settingsFile(),QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(350,500));
    }

    settings.endGroup();
}

