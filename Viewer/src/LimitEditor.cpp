//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LimitEditor.hpp"

#include <QtGlobal>
#include <QIntValidator>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

LimitEditorWidget::LimitEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

LimitEditor::LimitEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    w_=new LimitEditorWidget(this);
    addForm(w_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    w_->valueLe_->setClearButtonEnabled(true);
    w_->maxLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "limit");

    if(a->data().count() < 4)
       return;

    QString name=a->data().at(1);

    w_->nameLabel_->setText(name);
    w_->valueLe_->setText(a->data().at(2));
    w_->maxLe_->setText(a->data().at(3));

    if(a->data().at(2).isEmpty() || a->data().at(3).isEmpty())
    {
        return;
    }

    valOri_=a->data().at(2).toInt();
    maxOri_=a->data().at(3).toInt();

    QIntValidator *valValidator=new QIntValidator(this);
    valValidator->setRange(0,maxOri_);
    w_->valueLe_->setValidator(valValidator);

    QIntValidator *maxValidator=new QIntValidator(this);
    w_->valueLe_->setValidator(maxValidator);
    w_->valueLe_->setFocus();

    header_->setInfo(QString::fromStdString(info_->path()),"Limit");
}

void LimitEditor::apply()
{
    std::string val=w_->valueLe_->text().toStdString();
    std::string max=w_->maxLe_->text().toStdString();
    int intVal=w_->valueLe_->text().toInt();
    int intMax=w_->maxLe_->text().toInt();
    std::string name=w_->nameLabel_->text().toStdString();

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

static AttributeEditorMaker<LimitEditor> makerStr("limit");


