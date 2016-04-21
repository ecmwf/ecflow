//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "RepeatEditDialog.hpp"

#include <QtGlobal>
#include <QIntValidator>
#include <QStringListModel>

#include "Node.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "VInfo.hpp"
#include "VNode.hpp"
#include "VRepeat.hpp"

RepeatEditDialog::RepeatEditDialog(VInfo_ptr info,QWidget* parent) :
    AttributeEditor(info,parent),
    repeat_(0),
    model_(0)
{
    setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    valueLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "repeat");

    if(a->data().count() < 6)
        return;

    VNode *vnode=info_->node();
    Q_ASSERT(vnode);
    node_ptr node=vnode->node();
    Q_ASSERT(node);
    const Repeat& r=node->repeat();
    repeat_=VRepeat::make(r);

    QString name=a->data().at(1);
    nameLabel_->setText(name);
    valueLe_->setText(a->data().at(2));
    startLabel_->setText(a->data().at(3));
    endLabel_->setText(a->data().at(4));
    stepLabel_->setText(a->data().at(8));

    buildList();

#if 0
    QIntValidator *validator=new QIntValidator(this);
    if(!a->data().at(3).isEmpty() && !a->data().at(4).isEmpty())
    {
        validator->setRange(a->data().at(3).toInt(),
                            a->data().at(4).toInt());
    }
    valueLe_->setValidator(validator);
#endif

    header_->setInfo(QString::fromStdString(info_->path()),"Repeat");
}

RepeatEditDialog::~RepeatEditDialog()
{
    if(repeat_)
        delete repeat_;
}

void RepeatEditDialog::buildList()
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
        valueView_->setModel(model_);
        valueView_->setCurrentIndex(model_->index(current,0));

        connect(valueView_,SIGNAL(activated(const QModelIndex&)),
           this,SLOT(slotSelected(const QModelIndex&)));

        connect(valueLe_,SIGNAL(textEdited(QString)),
           this,SLOT(slotValueEdited(QString)));
    }
    else
    {
        valueView_->hide();
    }
}

void RepeatEditDialog::slotSelected(const QModelIndex& idx)
{
    valueLe_->setText(idx.data().toString());
}

void RepeatEditDialog::slotValueEdited(QString txt)
{
    if(isListMode())
    {
        int row=modelData_.indexOf(txt);
        if(row != -1)
        {
            valueView_->setCurrentIndex(model_->index(row,0));
        }
        else
        {
            valueView_->clearSelection();
            valueView_->setCurrentIndex(QModelIndex());
        }
    }
}

bool RepeatEditDialog::isListMode() const
{
    return (model_)?true:false;
}

void RepeatEditDialog::apply()
{
    std::string val=valueLe_->text().toStdString();
    std::string name=nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    std::string valType;
    switch(repeat_->type())
    {
    case VRepeat::StringType:
        valType="string";
        break;
    case VRepeat::IntType:
        valType="integer";
        break;
    default:
        break;
    }

    VAttribute::buildAlterCommand(cmd,"change","repeat",val);
    ServerHandler::command(info_,cmd);
}

static AttributeEditorMaker<RepeatEditDialog> makerStr("repeat");


