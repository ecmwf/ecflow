//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CommandOutputWidget.hpp"

#include <QDebug>
#include <QFontDatabase>

#include "ShellCommand.hpp"
#include "TextFormat.hpp"

CommandOutputWidget::CommandOutputWidget(QWidget *parent) :
  QWidget(parent),
  currentCommand_(0)
{
    setupUi(this);

    infoLabel_->setProperty("fileInfo","1");

    messageLabel_->hide();

    textEdit_->setShowLineNumbers(false);

    searchLine_->setEditor(textEdit_);
    searchLine_->setVisible(false);
}

CommandOutputWidget::~CommandOutputWidget()
{
}

bool CommandOutputWidget::addText(ShellCommand* cmd,QString txt)
{
    bool hasNewCmd=setCommand(cmd);
    textEdit_->appendPlainText(txt);
    return hasNewCmd;
}

bool CommandOutputWidget::addErrorText(ShellCommand* cmd,QString txt)
{
    bool hasNewCmd=setCommand(cmd);
    messageLabel_->appendError(txt);
    return hasNewCmd;
}

bool CommandOutputWidget::setCommand(ShellCommand* cmd)
{
    if(cmd != currentCommand_)
    {
        currentCommand_=cmd;
        textEdit_->clear();
        messageLabel_->clear();
        messageLabel_->hide();
        updateInfoLabel(currentCommand_);
        return true;
    }
    return false;
}

void CommandOutputWidget::updateInfoLabel(ShellCommand* cmd)
{
    if(!cmd)
    {
        infoLabel_->clear();
        return;
    }

    QColor boldCol(39,49,101);
    QColor defCol(90,90,90);
    QString s=Viewer::formatBoldText("Command: ",boldCol) + cmd->command() + "<br>" +
            Viewer::formatBoldText("Definition: ",boldCol) +
            Viewer::formatText(cmd->commandDef(),defCol) + "<br>" +
            Viewer::formatBoldText("Run at: ",boldCol) + cmd->startTime().toString("yyyy-MM-dd hh:mm:ss");

    infoLabel_->setText(s);
}

void CommandOutputWidget::removeSpacer()
{
    //Remove the first spacer item!!
    for(int i=0; horizontalLayout->count(); i++)
    {
        if(QSpacerItem* sp=horizontalLayout->itemAt(i)->spacerItem())
        {
            horizontalLayout->takeAt(i);
            delete sp;
            break;
        }
    }
}

void CommandOutputWidget::on_searchTb__clicked()
{
    searchLine_->setVisible(true);
    searchLine_->setFocus();
    searchLine_->selectAll();
}

void CommandOutputWidget::on_gotoLineTb__clicked()
{
    textEdit_->gotoLine();
}

void CommandOutputWidget::on_fontSizeUpTb__clicked()
{
    //We need to call a custom slot here instead of "zoomIn"!!!
    textEdit_->slotZoomIn();
}

void CommandOutputWidget::on_fontSizeDownTb__clicked()
{
    //We need to call a custom slot here instead of "zoomOut"!!!
    textEdit_->slotZoomOut();
}

