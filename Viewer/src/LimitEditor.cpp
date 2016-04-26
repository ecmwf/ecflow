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

#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

LimitEditorWidget::LimitEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

LimitEditor::LimitEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    w_=new LimitEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "limit");

    if(a->data().count() < 4)
       return;

    QString name=a->data().at(1);
    oriVal_=a->data().at(2).toInt();
    oriMax_=a->data().at(3).toInt();

    w_->nameLabel_->setText(name);
    w_->valueSpin_->setValue(oriVal_);
    w_->maxSpin_->setValue(oriMax_);

    w_->valueSpin_->setFocus();

    if(a->data().at(2).isEmpty() || a->data().at(3).isEmpty())
    {
        return;
    }

    connect(w_->valueSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotValueChanged(int)));

    connect(w_->maxSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotMaxChanged(int)));

    header_->setInfo(QString::fromStdString(info_->path()),"Limit");

    checkButtonStatus();

    readSettings();
}

void LimitEditor::apply()
{
    int intVal=w_->valueSpin_->value();
    int intMax=w_->maxSpin_->value();
    std::string val=QString::number(intVal).toStdString();
    std::string max=QString::number(intMax).toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> valCmd;
    VAttribute::buildAlterCommand(valCmd,"change","limit_value",name,val);

    std::vector<std::string> maxCmd;
    VAttribute::buildAlterCommand(maxCmd,"change","limit_max",name,max);

    if(oriVal_ != intVal && oriMax_ != intMax)
    {
        if(intVal < oriMax_)
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
    else if(oriVal_ != intVal)
    {
        ServerHandler::command(info_,valCmd);
    }

    else if(oriMax_ != intMax)
    {
        ServerHandler::command(info_,maxCmd);
    }
}

void LimitEditor::resetValue()
{
    w_->valueSpin_->setValue(oriVal_);
    w_->maxSpin_->setValue(oriMax_);
    checkButtonStatus();
}

void LimitEditor::slotValueChanged(int)
{
    checkButtonStatus();
}

void LimitEditor::slotMaxChanged(int)
{
    checkButtonStatus();
}

bool LimitEditor::isValueChanged()
{
    return (oriVal_ != w_->valueSpin_->value() || oriMax_ != w_->maxSpin_->value());
}

void LimitEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("LimitEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void LimitEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("LimitEditor")),
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

static AttributeEditorMaker<LimitEditor> makerStr("limit");
