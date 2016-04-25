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

#include <QtGlobal>
#include <QIntValidator>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

MeterEditorWidget::MeterEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

MeterEditor::MeterEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    w_=new MeterEditorWidget(this);
    addForm(w_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    w_->valueLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "meter");

    if(a->data().count() < 5)
            return;

    QString name=a->data().at(1);

    w_->nameLabel_->setText(name);
    w_->valueLe_->setText(a->data().at(2));
    w_->minLabel_->setText(a->data().at(3));
    w_->maxLabel_->setText(a->data().at(4));
    w_->thresholdLabel_->setText("???");

    QIntValidator *validator=new QIntValidator(this);
    if(!a->data().at(3).isEmpty() && !a->data().at(4).isEmpty())
    {
        validator->setRange(a->data().at(3).toInt(),
                            a->data().at(4).toInt());
    }
    w_->valueLe_->setValidator(validator);
    w_->valueLe_->setFocus();

    header_->setInfo(QString::fromStdString(info_->path()),"Meter");
}

void MeterEditor::apply()
{
    std::string val=w_->valueLe_->text().toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","meter",name,val);
    ServerHandler::command(info_,cmd);
}

static AttributeEditorMaker<MeterEditor> makerStr("meter");

