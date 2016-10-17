//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerEditor.hpp"

#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "Highlighter.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"

TriggerEditorWidget::TriggerEditorWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

#if 0
    QLayoutItem *item;
    item=grid_->itemAtPosition(1,0);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft|Qt::AlignTop);
#endif

    Highlighter *h=new Highlighter(te_->document(),"trigger");
    te_->setShowLineNumbers(false);
}

TriggerEditor::TriggerEditor(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,"trigger",parent)
{
    w_=new TriggerEditorWidget(this);
    addForm(w_);

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "trigger");

    //Data is built dynamically so we store it
    QStringList data=a->data();

    if(data.count() != 3)
        return;

    QString txt=data[2];

    oriText_=txt;
    w_->te_->setPlainText(txt);

    QString typeInHeader;
    if(data[1]=="0")
    {
        typeInCmd_="trigger";
        typeInHeader="Trigger";
    }
    else
    {
        typeInCmd_="complete";
        typeInHeader="Complete";
    }
    typeInHeader+=" expression";

    header_->setInfo(QString::fromStdString(info_->path()),typeInHeader);

    connect(w_->te_,SIGNAL(textChanged()),
            this,SLOT(slotValueChanged()));

    checkButtonStatus();

    readSettings();
}

TriggerEditor::~TriggerEditor()
{
    writeSettings();
}

void TriggerEditor::apply()
{
    if(typeInCmd_.isEmpty())
        return;

    std::string txt=w_->te_->toPlainText().toStdString();
    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change",typeInCmd_.toStdString(),txt);
    ServerHandler::command(info_,cmd);
}

void TriggerEditor::resetValue()
{
    w_->te_->setPlainText(oriText_);
    checkButtonStatus();
}

void TriggerEditor::slotValueChanged()
{
    checkButtonStatus();
}

bool TriggerEditor::isValueChanged()
{
    return (oriText_ != w_->te_->toPlainText());
}

void TriggerEditor::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TriggerEditor")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());
    settings.setValue("fontSize",w_->te_->font().pointSize());
    settings.endGroup();
}

void TriggerEditor::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TriggerEditor")),
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

    if(settings.contains("fontSize"))
    {
        QFont f=w_->te_->font();
        int fSize=settings.value("fontSize").toInt();
        if(fSize > 0 && fSize < 100)
            f.setPointSize(fSize);

        w_->te_->setFont(f);
    }

    settings.endGroup();
}

static AttributeEditorMaker<TriggerEditor> makerStr("trigger");

