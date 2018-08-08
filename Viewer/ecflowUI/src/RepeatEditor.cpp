//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include <QItemSelectionModel>
#include <QStringListModel>
#include <QSettings>

#include "Node.hpp"
#include "AttributeEditorFactory.hpp"
#include "CommandHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "SessionHandler.hpp"
#include "UiLog.hpp"
#include "VInfo.hpp"
#include "VNode.hpp"
#include "VRepeatAttr.hpp"

RepeatEditorWidget::RepeatEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

void RepeatEditorWidget::hideRow(QWidget* w)
{
    w->hide();
    QWidget* item=formLayout_->labelForField(w);
    Q_ASSERT(item);
    item->hide();
}

//================================================================
//
// RepeatEditor
//
//================================================================

RepeatEditor::RepeatEditor(VInfo_ptr info,QWidget* parent) :
    AttributeEditor(info,"repeat",parent),
    model_(nullptr)
{
    w_=new RepeatEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "repeat");

    auto *rep=static_cast<VRepeatAttr*>(a);

#if 0
    if(a->data().count() < 7)
        return;

    VNode *vnode=info_->node();
    Q_ASSERT(vnode);
    node_ptr node=vnode->node();
    Q_ASSERT(node);
    const Repeat& r=node->repeat();
    repeat_=VRepeat::make(r);
#endif


    oriVal_=a->data().at(3);

    w_->nameLabel_->setText(a->data().at(2));
    //w_->valueLe_->setText(a->data().at(3));
    w_->startLabel_->setText(a->data().at(4));
    w_->endLabel_->setText(a->data().at(5));
    w_->stepLabel_->setText(a->data().at(6));

    //Value will be initailised in the subclasses
    oriVal_=a->data().at(3);

    buildList(rep);

#if 0
    QIntValidator *validator=new QIntValidator(this);
    if(!a->data().at(3).isEmpty() && !a->data().at(4).isEmpty())
    {
        validator->setRange(a->data().at(3).toInt(),
                            a->data().at(4).toInt());
    }
    valueLe_->setValidator(validator);
#endif

    header_->setInfo(QString::fromStdString(info_->nodePath()),"Repeat " + QString::fromStdString(rep->subType()));

    readSettings();
}

RepeatEditor::~RepeatEditor()
{
    writeSettings();
}

void RepeatEditor::buildList(VRepeatAttr *rep)
{
    int start=rep->startIndex();
    int end=rep->endIndex();
    int step=rep->step();
    int current=rep->currentIndex();

    if(step<=0 || end <= start)
    {
        return;
    }

    modelData_.clear();
    int cnt=end-start;
    if(cnt >1)
    {
        for(int i=start; i <= end; i++)
            modelData_ << QString::fromStdString(rep->value(i));

        model_=new QStringListModel(this);
        model_->setStringList(modelData_);
        w_->valueView_->setModel(model_);
        w_->valueView_->setCurrentIndex(model_->index(current,0));

        connect(w_->valueView_->selectionModel(),
              SIGNAL(currentChanged(QModelIndex,QModelIndex)),
              this, SLOT(slotSelectedInView(QModelIndex,QModelIndex)));

        w_->valueView_->setFocus(Qt::MouseFocusReason);
    }
    else
    {
        w_->valueView_->hide();
    }
}

void RepeatEditor::slotSelectedInView(const QModelIndex &current, const QModelIndex &previous)
{
    setValue(current.data().toString());
    checkButtonStatus();
}

bool RepeatEditor::isListMode() const
{
    return (model_)?true:false;
}

void RepeatEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("RepeatEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void RepeatEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("RepeatEditor")),
                       QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(310,340));
    }

    settings.endGroup();
}

//================================================================
//
// RepeatIntEditor
//
//================================================================

RepeatIntEditor::RepeatIntEditor(VInfo_ptr info,QWidget* parent) :
    RepeatEditor(info,parent)
{
    //if(!repeat_)
    //    return;

    w_->hideRow(w_->valueLe_);

    initSpinner();

    connect(w_->valueSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotValueChanged(int)));

    checkButtonStatus();
}

void RepeatIntEditor::initSpinner()
{
    w_->valueSpin_->setValue(oriVal_.toInt());

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "repeat");

    auto *rep=static_cast<VRepeatAttr*>(a);

    int startIndex=rep->startIndex();
    int endIndex=rep->endIndex();
    int step=rep->step();

    if(step<=0 || endIndex <= startIndex)
    {
        return;
    }

    int minVal=QString::fromStdString(rep->value(startIndex)).toInt();
    int maxVal=QString::fromStdString(rep->value(endIndex)).toInt();

#if 0
    UiLog().dbg() << "min=" << minVal << " max=" << maxVal << " step=" << step;
#endif
    w_->valueSpin_->setMinimum(minVal);
    w_->valueSpin_->setMaximum(maxVal);
    w_->valueSpin_->setSingleStep(step);
}

void RepeatIntEditor::setValue(QString val)
{
   w_->valueSpin_->setValue(val.toInt());
}

void RepeatIntEditor::slotValueChanged(int val)
{
    if(isListMode())
    {
        QString txt=QString::number(val);
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

void RepeatIntEditor::resetValue()
{
    w_->valueSpin_->setValue(oriVal_.toInt());
    slotValueChanged(oriVal_.toInt());
    checkButtonStatus();
}

bool RepeatIntEditor::isValueChanged()
{
    return(oriVal_.toInt() != w_->valueSpin_->value());
}

void RepeatIntEditor::apply()
{
    std::string val=QString::number(w_->valueSpin_->value()).toStdString();
    //std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","repeat",val);
    CommandHandler::run(info_,cmd);
}

//================================================================
//
// RepeatStringEditor
//
//================================================================

RepeatStringEditor::RepeatStringEditor(VInfo_ptr info,QWidget* parent) :
    RepeatEditor(info,parent)
{
    //if(!repeat_)
    //    return;

    w_->hideRow(w_->valueSpin_);
    w_->hideRow(w_->stepLabel_);
    w_->valueLe_->setText(oriVal_);

    connect(w_->valueLe_,SIGNAL(textEdited(QString)),
       this,SLOT(slotValueEdited(QString)));

    checkButtonStatus();
}

void RepeatStringEditor::setValue(QString val)
{
   w_->valueLe_->setText(val);
}

void RepeatStringEditor::slotValueEdited(QString txt)
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

void RepeatStringEditor::resetValue()
{
    w_->valueLe_->setText(oriVal_);
    slotValueEdited(oriVal_);
    checkButtonStatus();
}

bool RepeatStringEditor::isValueChanged()
{
    return(oriVal_ != w_->valueLe_->text());
}

void RepeatStringEditor::apply()
{
    std::string val=w_->valueLe_->text().toStdString();
    //std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","repeat",val);
    CommandHandler::run(info_,cmd);
}

//================================================================
//
// RepeatDateEditor
//
//================================================================

RepeatDateEditor::RepeatDateEditor(VInfo_ptr info,QWidget* parent) :
    RepeatEditor(info,parent)
{
    //if(!repeat_)
    //    return;

    w_->hideRow(w_->valueSpin_);
    w_->valueLe_->setText(oriVal_);

    connect(w_->valueLe_,SIGNAL(textEdited(QString)),
       this,SLOT(slotValueEdited(QString)));

    checkButtonStatus();
}

void RepeatDateEditor::setValue(QString val)
{
   w_->valueLe_->setText(val);
}

void RepeatDateEditor::slotValueEdited(QString txt)
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

void RepeatDateEditor::resetValue()
{
    w_->valueLe_->setText(oriVal_);
    slotValueEdited(oriVal_);
    checkButtonStatus();
}

bool RepeatDateEditor::isValueChanged()
{
    return(oriVal_ != w_->valueLe_->text());
}

void RepeatDateEditor::apply()
{
    std::string val=w_->valueLe_->text().toStdString();
    //std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","repeat",val);
    CommandHandler::run(info_,cmd);
}


static AttributeEditorMaker<RepeatIntEditor> makerStr1("repeat_integer");
static AttributeEditorMaker<RepeatStringEditor> makerStr2("repeat_string");
static AttributeEditorMaker<RepeatStringEditor> makerStr3("repeat_enumerated");
static AttributeEditorMaker<RepeatDateEditor> makerStr4("repeat_date");
