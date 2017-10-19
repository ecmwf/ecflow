//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMMANDOUTPUTWIDGET_HPP
#define COMMANDOUTPUTWIDGET_HPP

#include <QWidget>

#include "ui_CommandOutputWidget.h"

class PlainTextEdit;
class ShellCommand;

class CommandOutputWidget : public QWidget, protected Ui::CommandOutputWidget
{
Q_OBJECT

public:
    explicit CommandOutputWidget(QWidget *parent=0);
    ~CommandOutputWidget();

    bool addText(ShellCommand* cmd,QString txt);
    bool addErrorText(ShellCommand* cmd,QString txt);

protected Q_SLOTS:
    void on_searchTb__clicked();
    void on_gotoLineTb__clicked();
    void on_fontSizeUpTb__clicked();
    void on_fontSizeDownTb__clicked();

Q_SIGNALS:
    void editorFontSizeChanged();

protected:
    bool setCommand(ShellCommand* cmd);
    void removeSpacer();
    void updateInfoLabel(ShellCommand* cmd);

    ShellCommand* currentCommand_;
};

#endif // COMMANDOUTPUTWIDGET_HPP

