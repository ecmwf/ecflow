//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LimitEditor.hpp"

#include <QItemSelectionModel>
#include <QSettings>
#include <QStringListModel>

#include "Aspect.hpp"

#include "AttributeEditorFactory.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VLimitAttr.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

LimitEditorWidget::LimitEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
    removeTb_->setDefaultAction(actionRemove_);
    removeAllTb_->setDefaultAction(actionRemoveAll_);
    pathView_->addAction(actionRemove_);
    pathView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pathView_->setContextMenuPolicy(Qt::ActionsContextMenu);
}

LimitEditor::LimitEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,"limit",parent), model_(0)
{
    w_=new LimitEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "limit");
    QStringList aData=a->data();

    if(aData.count() < 4)
       return;

    QString name=aData[1];
    oriVal_=aData[2].toInt();
    oriMax_=aData[3].toInt();

    w_->nameLabel_->setText(name);
    w_->valueLabel_->setText(QString::number(oriVal_));
    w_->maxSpin_->setValue(oriMax_);

    w_->maxSpin_->setRange(0,10000);
    w_->maxSpin_->setFocus();

    if(aData[2].isEmpty() || aData[3].isEmpty())
    {
        return;
    }

    buildList(a);

    connect(w_->maxSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotMaxChanged(int)));

    connect(w_->actionRemove_,SIGNAL(triggered()),
            this,SLOT(slotRemove()));

    connect(w_->actionRemoveAll_,SIGNAL(triggered()),
            this,SLOT(slotRemoveAll()));

    header_->setInfo(QString::fromStdString(info_->path()),"Limit");

    checkButtonStatus();

    readSettings();

    //No reset button is allowed because we can perform irreversible changes!
    doNotUseReset();
}

LimitEditor::~LimitEditor()
{
    writeSettings();
}

void LimitEditor::buildList(VAttribute *a)
{
    VLimitAttr* lim=static_cast<VLimitAttr*>(a);
    Q_ASSERT(lim);

    modelData_.clear();
    modelData_=lim->paths();

    if(modelData_.count() > 0)
    {
        model_=new QStringListModel(this);
        model_->setStringList(modelData_);

        w_->pathView_->setModel(model_);
        w_->pathView_->setCurrentIndex(model_->index(0,0));
        w_->pathView_->setFocus(Qt::MouseFocusReason);
    }
    else
    {
        w_->actionRemove_->setEnabled(false);
        w_->actionRemoveAll_->setEnabled(false);
    }
}

void LimitEditor::apply()
{
    int intVal=w_->valueLabel_->text().toInt();
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
}

void LimitEditor::slotMaxChanged(int)
{
    checkButtonStatus();
}

bool LimitEditor::isValueChanged()
{
    return (oriVal_ != w_->valueLabel_->text().toInt() || oriMax_ != w_->maxSpin_->value());
}

void LimitEditor::slotRemove()
{
    remove(false);
}

void LimitEditor::slotRemoveAll()
{
    remove(true);
}

void LimitEditor::remove(bool all)
{
    if(!info_)
        return;

    //We cannot cancle the setting after remove is callled
    disableCancel();

    Q_ASSERT(model_);

    VAttribute* a=info_->attribute();
    Q_ASSERT(a);
    VLimitAttr* lim=static_cast<VLimitAttr*>(a);
    Q_ASSERT(lim);

    if(all)
    {
        std::vector<std::string> valCmd;
        VAttribute::buildAlterCommand(valCmd,"change","limit_value",a->strName(),"0");
        ServerHandler::command(info_,valCmd);
    }
    else
    {
        std::vector<std::string> paths;
        Q_FOREACH(QModelIndex idx,w_->pathView_->selectionModel()->selectedRows())
        {
            std::vector<std::string> valCmd;
            VAttribute::buildAlterCommand(valCmd,"delete","limit_path",a->strName(),
                                          model_->data(idx,Qt::DisplayRole).toString().toStdString());
            ServerHandler::command(info_,valCmd);
        }
    }

    //Updating the gui with the new state will happen later
    //because command() is asynchronous
}

void LimitEditor::nodeChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
    bool limitCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::LIMIT) != aspect.end());
    if(limitCh && info_)
    {
        VAttribute* a=info_->attribute();
        Q_ASSERT(a);
        VLimitAttr* lim=static_cast<VLimitAttr*>(a);
        Q_ASSERT(lim);

        QStringList aData=a->data();
        if(aData.count() < 4)
           return;

        oriVal_=aData[2].toInt();
        w_->valueLabel_->setText(QString::number(oriVal_));
        modelData_.clear();
        modelData_=lim->paths();
        model_->setStringList(modelData_);
    }
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
        resize(QSize(420,400));
    }

    settings.endGroup();
}

static AttributeEditorMaker<LimitEditor> makerStr("limit");
