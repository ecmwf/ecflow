//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "RepeatEditor.hpp"

#include <QtGlobal>
#include <QIntValidator>
#include <QStringListModel>

#include "Node.hpp"
#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "VInfo.hpp"
#include "VNode.hpp"
#include "VRepeat.hpp"

RepeatEditorWidget::RepeatEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

RepeatEditor::RepeatEditor(VInfo_ptr info,QWidget* parent) :
    AttributeEditor(info,parent),
    repeat_(0),
    model_(0)
{
    w_=new RepeatEditorWidget(this);
    addForm(w_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    w_->valueLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "repeat");

    if(a->data().count() < 7)
        return;

    VNode *vnode=info_->node();
    Q_ASSERT(vnode);
    node_ptr node=vnode->node();
    Q_ASSERT(node);
    const Repeat& r=node->repeat();
    repeat_=VRepeat::make(r);

    oriVal_=a->data().at(3);
    w_->nameLabel_->setText(a->data().at(2));
    w_->valueLe_->setText(a->data().at(3));
    w_->startLabel_->setText(a->data().at(4));
    w_->endLabel_->setText(a->data().at(5));

    if(repeat_->valueType() == VRepeat::StringType)
    {
        w_->stepNameLabel_->hide();
        w_->stepLabel_->hide();
    }
    else
    {
        w_->stepLabel_->setText(a->data().at(6));
    }
    oriVal_=a->data().at(3);

    buildList();

    connect(w_->resetTb_,SIGNAL(clicked()),
            this,SLOT(slotResetValue()));

#if 0
    QIntValidator *validator=new QIntValidator(this);
    if(!a->data().at(3).isEmpty() && !a->data().at(4).isEmpty())
    {
        validator->setRange(a->data().at(3).toInt(),
                            a->data().at(4).toInt());
    }
    valueLe_->setValidator(validator);
#endif

    header_->setInfo(QString::fromStdString(info_->path()),"Repeat " + QString::fromStdString(repeat_->type()));

    checkButtonStatus();
}

RepeatEditor::~RepeatEditor()
{
    if(repeat_)
        delete repeat_;
}

void RepeatEditor::buildList()
{
    int start=repeat_->startIndex();
    int end=repeat_->endIndex();
    int step=repeat_->step();
    int current=repeat_->currentIndex();

    if(step<=0 || end <= start)
    {
        return;
    }

    modelData_.clear();
    int cnt=end-start;
    if(cnt >1 && cnt < 100)
    {
        for(size_t i=start; i <= end; i++)
            modelData_ << QString::fromStdString(repeat_->value(i));

        model_=new QStringListModel(this);
        model_->setStringList(modelData_);
        w_->valueView_->setModel(model_);
        w_->valueView_->setCurrentIndex(model_->index(current,0));

        connect(w_->valueView_,SIGNAL(activated(const QModelIndex&)),
           this,SLOT(slotSelected(const QModelIndex&)));

        connect(w_->valueLe_,SIGNAL(textEdited(QString)),
           this,SLOT(slotValueEdited(QString)));
    }
    else
    {
        w_->valueView_->hide();
    }
}

void RepeatEditor::slotSelected(const QModelIndex& idx)
{
    w_->valueLe_->setText(idx.data().toString());
    checkButtonStatus();
}

void RepeatEditor::slotValueEdited(QString txt)
{
    if(isListMode())
    {
        int row=modelData_.indexOf(txt);
        if(row != -1)
        {
            w_->valueView_->setCurrentIndex(model_->index(row,0));
        }
        else
        {
            w_->valueView_->clearSelection();
            w_->valueView_->setCurrentIndex(QModelIndex());
        }
    }
    checkButtonStatus();
}

void RepeatEditor::slotResetValue()
{
    w_->valueLe_->setText(oriVal_);
    slotValueEdited(oriVal_);
    checkButtonStatus();
}

void RepeatEditor::checkButtonStatus()
{
    w_->resetTb_->setEnabled(oriVal_ != w_->valueLe_->text());
}

bool RepeatEditor::isListMode() const
{
    return (model_)?true:false;
}

void RepeatEditor::apply()
{
    std::string val=w_->valueLe_->text().toStdString();
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","repeat",val);
    ServerHandler::command(info_,cmd);
}

static AttributeEditorMaker<RepeatEditor> makerStr("repeat");


