//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "EventEditor.hpp"

#include <QItemSelectionModel>
#include <QSettings>
#include <QShowEvent>
#include <QStringListModel>
#include <QTimer>

#include "Aspect.hpp"

#include "AttributeEditorFactory.hpp"
#include "CommandHandler.hpp"
#include "MainWindow.hpp"
#include "TriggeredScanner.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VEventAttr.hpp"
#include "VNode.hpp"
#include "SessionHandler.hpp"

EventEditorWidget::EventEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    pathView_->addAction(actionLookUp_);
    pathView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pathView_->setContextMenuPolicy(Qt::ActionsContextMenu);
}

EventEditor::EventEditor(VInfo_ptr info,QWidget* parent) :
    AttributeEditor(info,"event",parent),
    model_(0),
    scanned_(false)
{
    w_=new EventEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "event");
    QStringList aData=a->data();

    if(aData.count() < 3)
       return;

    QString name=aData[1];
    oriVal_=aData[2].toInt();

    w_->nameLabel_->setText(name);
    w_->statusSetRb_->setChecked(oriVal_ == 1);
    w_->statusClearRb_->setChecked(oriVal_ != 1);

    w_->messageLabel_->hide();
    w_->messageLabel_->setShowTypeTitle(false);

    connect(w_->actionLookUp_,SIGNAL(triggered()),
            this,SLOT(slotLookUp()));

    connect(w_->pathView_,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClicked(const QModelIndex&)));

    connect(w_->statusSetRb_,SIGNAL(toggled(bool)),
            this,SLOT(slotStatusToggled(bool)));

    header_->setInfo(QString::fromStdString(info_->path()),"Event");

    checkButtonStatus();

    readSettings();

    //No reset button is allowed because we can perform irreversible changes!
    doNotUseReset();
}

EventEditor::~EventEditor()
{
    writeSettings();
}

//This was the only way to guarantee that the trigger scanning is called
//after the dialogue appears on screen (showEvent did not work!)
void EventEditor::paintEvent(QPaintEvent *e)
{
    QDialog::paintEvent(e);
    if(!scanned_)
    {
         QTimer::singleShot(0, this, SLOT(buildList()));
    }
}

//We only build the list when the dialogoue is already visible so that
//the progressbar could show the scanning process
void EventEditor::buildList()
{   
    if(!info_)
        return;

    VAttribute* a=info_->attribute();
    if(!a)
        return;

    if(!model_)
    {
        model_=new QStringListModel(this);
        w_->pathView_->setModel(model_);
    }

    if(VNode* node=info_->node())
    {
        TriggeredScanner* scanner=new TriggeredScanner(this);

        connect(scanner,SIGNAL(scanStarted()),
                this,SLOT(scanStarted()));

        connect(scanner,SIGNAL(scanFinished()),
                this,SLOT(scanFinished()));

        connect(scanner,SIGNAL(scanProgressed(int)),
                this,SLOT(scanProgressed(int)));

        std::vector<std::string> v;
        node->triggeredByEvent(a->strName(),v,scanner);

        w_->messageLabel_->hide();

        //Update the model(=node list)
        QStringList lst;
        for(size_t i=0; i < v.size(); i++)
            lst << QString::fromStdString(v[i]);

        setModelData(lst);

        delete scanner;

        scanned_=true;
    }
}

void EventEditor::scanStarted()
{
    w_->messageLabel_->showInfo("Scanning dependencies based on this event ...");
    w_->messageLabel_->startProgress(100);
}

void EventEditor::scanFinished()
{
    w_->messageLabel_->stopProgress();
    w_->messageLabel_->hide();
}

void EventEditor::scanProgressed(int value)
{
    std::string text="";
    w_->messageLabel_->progress(QString::fromStdString(text),value);
}

void EventEditor::apply()
{
    int intVal=(w_->statusSetRb_->isChecked())?1:0;
    std::string val=(w_->statusSetRb_->isChecked())?"set":"clear";
    std::string name=w_->nameLabel_->text().toStdString();

    std::vector<std::string> valCmd;
    VAttribute::buildAlterCommand(valCmd,"change","event",name,val);

    if(oriVal_ != intVal)
    {
        CommandHandler::run(info_,valCmd);
    }
}

void EventEditor::resetValue()
{
}

void EventEditor::slotStatusToggled(bool)
{
    checkButtonStatus();
}

bool EventEditor::isValueChanged()
{
    return (oriVal_ != (w_->statusSetRb_->isChecked()?1:0));
}

void EventEditor::nodeChanged(const std::vector<ecf::Aspect::Type>& aspect)
{
    bool eventCh=(std::find(aspect.begin(),aspect.end(),ecf::Aspect::EVENT) != aspect.end());
    if(eventCh && info_)
    {
        VAttribute* a=info_->attribute();
        Q_ASSERT(a);

        QStringList aData=a->data();
        if(aData.count() < 3)
            return;

        oriVal_=aData[2].toInt();
        w_->statusSetRb_->setChecked(oriVal_==1);

        //Update the model (=node list)
        //setModelData(lim->paths());
    }
}

void EventEditor::setModelData(QStringList lst)
{
    Q_ASSERT(model_);

    bool hadData=(modelData_.isEmpty() == false);
    modelData_=lst;
    model_->setStringList(modelData_);

    if(!modelData_.isEmpty())
    {
        if(!hadData)
        {
            w_->pathView_->setCurrentIndex(model_->index(0,0));
            w_->pathView_->setFocus(Qt::MouseFocusReason);
        }
    }
}
//Lookup in tree

void EventEditor::slotLookUp()
{
     QModelIndex idx=w_->pathView_->currentIndex();
     lookup(idx);
}

void EventEditor::slotDoubleClicked(const QModelIndex &index)
{
    lookup(index);
}

void EventEditor::lookup(const QModelIndex &idx)
{
    if(!info_)
        return;

    Q_ASSERT(model_);

    std::string nodePath=
            model_->data(idx,Qt::DisplayRole).toString().toStdString();

    VInfo_ptr ni=VInfo::createFromPath(info_->server(),nodePath);
    if(ni)
    {
        MainWindow::lookUpInTree(ni);
    }
}

void EventEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("EventEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.endGroup();
}

void EventEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("EventEditor")),
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

static AttributeEditorMaker<EventEditor> makerStr("event");
