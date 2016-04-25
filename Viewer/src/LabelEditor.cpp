//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LabelEditor.hpp"

#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

LabelEditorWidget::LabelEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    QLayoutItem *item;
    item=grid_->itemAtPosition(1,0);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    item=grid_->itemAtPosition(1,2);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft|Qt::AlignTop);

}

LabelEditor::LabelEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    w_=new LabelEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "label");

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

    header_->setInfo(QString::fromStdString(info_->path()),"Label");

    connect(w_->valueTe_,SIGNAL(textChanged()),
            this,SLOT(slotValueChanged()));

    connect(w_->resetTb_,SIGNAL(clicked()),
            this,SLOT(slotResetValue()));

    checkButtonStatus();

    readSettings();
}

LabelEditor::~LabelEditor()
{
    writeSettings();
}

void LabelEditor::apply()
{
    std::string val=w_->valueTe_->toPlainText().toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","label",name,val);
    ServerHandler::command(info_,cmd);
}

void LabelEditor::slotResetValue()
{
    w_->valueTe_->setPlainText(oriVal_);
    checkButtonStatus();
}

void LabelEditor::slotValueChanged()
{
    checkButtonStatus();
}

void LabelEditor::checkButtonStatus()
{
    w_->resetTb_->setEnabled(oriVal_ != w_->valueTe_->toPlainText());
}

void LabelEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("LabelEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void LabelEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("LabelEditor")),
                       QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(300,200));
    }

    settings.endGroup();
}

static AttributeEditorMaker<LabelEditor> makerStr("label");
