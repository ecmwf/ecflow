//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VariableEditor.hpp"

#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VGenVarAttr.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

VariableEditorWidget::VariableEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    QLayoutItem *item;
    item=grid_->itemAtPosition(1,0);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft|Qt::AlignTop);
}

VariableEditor::VariableEditor(VInfo_ptr info,QWidget* parent) :
    AttributeEditor(info,"variable",parent),
    readOnly_(false)
{
    w_=new VariableEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "var" || a->type()->name() == "genvar");

    if(a->data().count() < 2)
        return;

    QString name=a->data().at(1);
    QString val;
    if(a->data().count() > 2)
        val=a->data().at(2);

    oriVal_=val;

    w_->nameLabel_->setText(name);
    w_->valueTe_->setPlainText(val);
    w_->valueTe_->setFocus();

    QString typeLabel=(a->type()->name() == "var")?"User variable":"Generated variable";
    readOnly_=VGenVarAttr::isReadOnly(name.toStdString());
    if(readOnly_)
    {
        w_->valueTe_->setReadOnly(true);
        typeLabel+=" (read only)";
    }
    else
    {
        connect(w_->valueTe_,SIGNAL(textChanged()),
            this,SLOT(slotValueChanged()));
    }

    header_->setInfo(QString::fromStdString(info_->path()),typeLabel);

    checkButtonStatus();

    readSettings();
}

VariableEditor::~VariableEditor()
{
    writeSettings();
}

void VariableEditor::apply()
{
    if(!readOnly_)
    {
        std::string val=w_->valueTe_->toPlainText().toStdString();
        std::string name=w_->nameLabel_->text().toStdString();

        if(val != oriVal_.toStdString())
        {
            std::vector<std::string> cmd;
            VAttribute::buildAlterCommand(cmd,"change","variable",name,val);
            ServerHandler::command(info_,cmd);
        }
    }
}

void VariableEditor::resetValue()
{
    w_->valueTe_->setPlainText(oriVal_);
    checkButtonStatus();
}

void VariableEditor::slotValueChanged()
{
    checkButtonStatus();
}

bool VariableEditor::isValueChanged()
{
    return (oriVal_ != w_->valueTe_->toPlainText());
}

void VariableEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("VariableEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void VariableEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("VariableEditor")),
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

static AttributeEditorMaker<VariableEditor> makerStrVar("var");
static AttributeEditorMaker<VariableEditor> makerStrGenvar("genvar");
