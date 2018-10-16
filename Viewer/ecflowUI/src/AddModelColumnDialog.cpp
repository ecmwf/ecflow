//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "AddModelColumnDialog.hpp"
#include "ModelColumn.hpp"

#include <QCompleter>
#include <QMessageBox>

#include "ui_AddModelColumnDialog.h"

AddModelColumnDialog::AddModelColumnDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddModelColumnDialog)
{
    ui->setupUi(this);
}

AddModelColumnDialog::~AddModelColumnDialog()
{
    delete ui;
}

void AddModelColumnDialog::init(ModelColumn* mc,const std::set<std::string>& vars,QString defaultText)
{
    modelColumn_=mc;
    QStringList varLst;
    for(std::set<std::string>::const_iterator it=vars.begin();
        it != vars.end(); ++it)
    {
        int idx=-1;
        bool ok=true;
        QString n=QString::fromStdString(*it);
        if((idx=modelColumn_->indexOf(n)) != -1)
            ok=!modelColumn_->isExtra(idx);

        if(ok)
            varLst << n;
    }

    QCompleter *c=new QCompleter(varLst,this);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    ui->variableLe->setCompleter(c);
    ui->variableLe->setText(defaultText);
}

void AddModelColumnDialog::accept()
{
    QString name=ui->variableLe->text();
    if(modelColumn_->indexOf(name) == -1)
    {
        modelColumn_->addExtraItem(name,name);
    }
    else
    {
        QMessageBox::warning(this,"Column already defined",tr("Column \'") + name +
                             tr("\' is already added to table view. Please choose another variable!"),
                             QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }

    QDialog::accept();
}

ChangeModelColumnDialog::ChangeModelColumnDialog(QWidget *parent) :
    AddModelColumnDialog(parent)
{
    setWindowTitle(tr("Change column in table view"));
}


void ChangeModelColumnDialog::setColumn(QString cname)
{
    columnName_=cname;
    ui->variableLe->setText(columnName_);
    setWindowTitle(tr("Change column ") + columnName_ + tr(" in table view"));
}

void ChangeModelColumnDialog::accept()
{
    QString name=ui->variableLe->text();
    if(modelColumn_->indexOf(name) == -1)
    {
        int colIdx=modelColumn_->indexOf(columnName_);
        if(colIdx != -1)
        {
            modelColumn_->changeExtraItem(colIdx,name,name);
        }
    }
    else
    {
        QMessageBox::warning(this,"Column already defined",tr("Column <b>") + name +
                             tr("</b> is already added to table view. Please choose another variable!"),
                             QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }

    QDialog::accept();
}
