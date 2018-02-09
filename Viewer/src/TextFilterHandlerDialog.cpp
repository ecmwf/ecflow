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
#include <QMessageBox>
#include <QSettings>
#include <QTableWidgetItem>

//======================================
//
// TextFilterAddDialog
//
//======================================

TextFilterAddDialog::TextFilterAddDialog(QWidget *parent) :
   QDialog(parent)
{
    setupUi(this);
}

void TextFilterAddDialog::init(QString name,QString filter)
{
    nameEdit_->setText(name);
    filterEdit_->setText(filter);
}

void TextFilterAddDialog::accept()
{
    QString name=nameEdit_->text();
    QString filter=filterEdit_->text();
#if 0
    if(TextFilterHandler::Instance()->contains(name.toStdString(),filter.toStdString()))
    {
        QMessageBox::critical(0,tr("Text filter"), "Cannot add text filter! Text filter with the same name: <b>"  +
                              name + "</b> and regexp: <b>" + filter + "</b> already exists!");
        return;
    }

    TextFilterHandler::Instance()->add(name.toStdString(),filter.toStdString());
#endif

    QDialog::accept();
}


//======================================
//
// TextFilterHandlerDialog
//
//======================================

TextFilterHandlerDialog::TextFilterHandlerDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    QAction *sep1=new QAction(this);
    sep1->setSeparator(true);

    QAction *sep2=new QAction(this);
    sep2->setSeparator(true);

    table_->addAction(actionAdd_);
    table_->addAction(sep1);
    table_->addAction(actionDuplicate_);
    table_->addAction(actionEdit_);
    table_->addAction(sep2);
    table_->addAction(actionRemove_);

    //Add actions for the toolbuttons
    addTb_->setDefaultAction(actionAdd_);
    editTb_->setDefaultAction(actionEdit_);
    removeTb_->setDefaultAction(actionRemove_);
    duplicateTb_->setDefaultAction(actionDuplicate_);

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
    headers << "Match mode" << "Case sensitive" << "Context menu" << "Regexp (grep)";
    table_->setHorizontalHeaderLabels(headers);

    const std::vector<TextFilterItem>& items=TextFilterHandler::Instance()->items();
    table_->setRowCount(items.size());
    for(size_t i=0; i < items.size(); i++)
    {
        QTableWidgetItem *filterItem = new QTableWidgetItem(QString::fromStdString(items[i].filter()));
        QTableWidgetItem *matchedItem = new QTableWidgetItem((items[i].matched())?"matched":"unmatched");
        QTableWidgetItem *caseItem = new QTableWidgetItem((items[i].caseSensitive())?"yes":"no");
        QTableWidgetItem *contextItem = new QTableWidgetItem((items[i].contextMenu())?"yes":"no");

        table_->setItem(i, 0, matchedItem);
        table_->setItem(i, 1, caseItem);
        table_->setItem(i, 2, contextItem);
        table_->setItem(i, 3, filterItem);
    }

    if(table_->rowCount() > 0)
        table_->setCurrentCell(0,0);

    updateStatus();
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

void TextFilterHandlerDialog::on_actionAdd__triggered()
{

}

#if 0
bool TextFilterHandlerDialog::addItem(QString name,QString filter)
{
    if(TextFilterHandler::Instance()->contains(name,filter))
    {
        return false;
    }

    if(TextFilterHandler::Instance()->add(name,filter))
    {
        reloadTable();
        return true;
    }
    return false;
}
#endif


#if 0
void TextFilterHandlerDialog::on_saveAsPb__clicked()
{
    std::string name=addNameLe_->text().toStdString();
    std::string filter=addFilterLe_->text().toStdString();
    bool matchMode=(addMatchedCb_->currentIndex()==0);
    bool caseSensitive=addCaseCb_->isChecked();

    if(TextFilterHandler::Instance()->contains(name,filter,matchMode,caseSensitive))
    {
        return;
    }

    TextFilterHandler::Instance()->add(name,filter,matchMode,caseSensitive);
    accept();
    //reloadTable();
}
#endif

void TextFilterHandlerDialog::updateStatus()
{
    bool hasSelected=table_->currentRow() >=0;

    actionEdit_->setEnabled(hasSelected);
    actionDuplicate_->setEnabled(hasSelected);
    actionRemove_->setEnabled(hasSelected);
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
        resize(QSize(520,330));
    }

    settings.endGroup();
}

