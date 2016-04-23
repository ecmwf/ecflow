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

#include <QtGlobal>

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

LabelEditWidget::LabelEditWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

LabelEditor::LabelEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    w_=new LabelEditWidget(this);
    addForm(w_);

//#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
//    w_->valueTe_->setClearButtonEnabled(true);
//#endif

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

    w_->nameLabel_->setText(name);
    w_->valueTe_->setPlainText(val);

    header_->setInfo(QString::fromStdString(info_->path()),"Label");
}

void LabelEditor::apply()
{
    std::string val=w_->valueTe_->toPlainText().toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","label",name,val);
    ServerHandler::command(info_,cmd);
}

static AttributeEditorMaker<LabelEditor> makerStr("label");
