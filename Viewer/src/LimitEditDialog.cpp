//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LimitEditDialog.hpp"

#include <QtGlobal>
#include <QIntValidator>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

LimitEditDialog::LimitEditDialog(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    valueLe_->setClearButtonEnabled(true);
    maxLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "limit");

    if(a->data().count() < 4)
       return;

    QString name=a->data().at(1);

    nameLabel_->setText(name);
    valueLe_->setText(a->data().at(2));
    maxLe_->setText(a->data().at(3));

    if(a->data().at(2).isEmpty() || a->data().at(3).isEmpty())
    {
        return;
    }

    valOri_=a->data().at(2).toInt();
    maxOri_=a->data().at(3).toInt();

    QIntValidator *valValidator=new QIntValidator(this);
    valValidator->setRange(0,maxOri_);
    valueLe_->setValidator(valValidator);

    QIntValidator *maxValidator=new QIntValidator(this);
    valueLe_->setValidator(maxValidator);

    header_->setInfo(QString::fromStdString(info_->path()),"Limit");
}

void LimitEditDialog::apply()
{
    std::string val=valueLe_->text().toStdString();
    std::string max=maxLe_->text().toStdString();
    int intVal=valueLe_->text().toInt();
    int intMax=maxLe_->text().toInt();
    std::string name=nameLabel_->text().toStdString();

    std::vector<std::string> valCmd;
    VAttribute::buildAlterCommand(valCmd,"change","limit_value",name,val);

    std::vector<std::string> maxCmd;
    VAttribute::buildAlterCommand(maxCmd,"change","limit_max",name,max);

    if(valOri_ != intVal && maxOri_ != intMax)
    {
        if(intVal < maxOri_)
        {
            ServerHandler::command(info_,valCmd);
            ServerHandler::command(info_,maxCmd);
        }
        else
        {
            ServerHandler::command(info_,maxCmd);
            ServerHandler::command(info_,valCmd);
        }
    }
    else if(valOri_ != intVal)
    {
        ServerHandler::command(info_,valCmd);
    }

    if(maxOri_ != intMax)
    {
        ServerHandler::command(info_,maxCmd);
    }
}

static AttributeEditorMaker<LimitEditDialog> makerStr("limit");


