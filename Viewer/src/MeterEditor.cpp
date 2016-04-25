//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "MeterEditor.hpp"

#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

MeterEditorWidget::MeterEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

MeterEditor::MeterEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent), oriVal_(0)
{
    w_=new MeterEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "meter");

    if(a->data().count() < 5)
        return;

    oriVal_=a->data().at(2).toInt();
    int min=a->data().at(3).toInt();
    int max=a->data().at(4).toInt();

    if(min > max || oriVal_ < min || oriVal_ > max)
        return;

    w_->nameLabel_->setText(a->data().at(1));
    w_->valueSpin_->setValue(oriVal_);
    w_->valueSpin_->setRange(min,max);
    w_->minLabel_->setText(a->data().at(3));
    w_->maxLabel_->setText(a->data().at(4));
    w_->thresholdLabel_->setText(a->data().at(5));

#if 0    //QString name=a->data().at(1);

    QIntValidator *validator=new QIntValidator(this);
    if(!a->data().at(3).isEmpty() && !a->data().at(4).isEmpty())
    {
        validator->setRange(a->data().at(3).toInt(),
                            a->data().at(4).toInt());
    }
    w_->valueLe_->setValidator(validator);
#endif
    w_->valueSpin_->setFocus();

    header_->setInfo(QString::fromStdString(info_->path()),"Meter");

    connect(w_->valueSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotValueChanged(int)));

    connect(w_->resetTb_,SIGNAL(clicked()),
            this,SLOT(slotResetValue()));

    checkButtonStatus();

    readSettings();
}

MeterEditor::~MeterEditor()
{
    writeSettings();
}

void MeterEditor::apply()
{
    std::string val=QString::number(w_->valueSpin_->value()).toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","meter",name,val);
    ServerHandler::command(info_,cmd);
}

void MeterEditor::slotResetValue()
{
    w_->valueSpin_->setValue(oriVal_);
    checkButtonStatus();
}

void MeterEditor::slotValueChanged(int)
{
    checkButtonStatus();
}

void MeterEditor::checkButtonStatus()
{
    w_->resetTb_->setEnabled(oriVal_ != w_->valueSpin_->value());
}

void MeterEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("MeterEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void MeterEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("MeterEditor")),
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





static AttributeEditorMaker<MeterEditor> makerStr("meter");

