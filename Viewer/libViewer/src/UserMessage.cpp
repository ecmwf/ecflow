
//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <iostream>

#include <QMessageBox>

#include "UserMessage.hpp"

#include "UiLog.hpp"

bool UserMessage::echoToCout_ = true;  // XXX should be false to start with

UserMessage::UserMessage()
{
}

void UserMessage::message(MessageType type, bool popup, const std::string& message)
{
    if (echoToCout_)
    {
#if 0
        switch (type)
        {
            case INFO:  std::cout << "INFO  : "; break;
            case WARN:  std::cout << "WARN  : "; break;
            case ERROR: std::cout << "ERROR : "; break;
            case DBG:   std::cout << "DEBUG : "; break;
            default:    std::cout << "      : "; break;
        }

        std::cout << message << std::endl;
#endif

        switch (type)
        {
        case DBG:   UiLog().dbg() << message; break;
        case INFO:  UiLog().info() << message ; break;
        case WARN:  UiLog().warn() << message; break;
        case ERROR: UiLog().err() << message; break;
        default:   break;
        }
    }

    if (popup)
    {
        QMessageBox::Icon icon;
        QString title;
        switch (type)
        {
            case INFO:  icon = QMessageBox::Information; title="Info"; break;
            case WARN:  icon = QMessageBox::Warning;     title="Warning";    break;
            case ERROR: icon = QMessageBox::Critical;    title="Error";   break;
            case DBG:   icon = QMessageBox::NoIcon;      title="Debug";   break;
            default:    icon = QMessageBox::NoIcon;      title="Info";     break;
        }
        title="ecFlowUI - " + title;
        QString qmsg = QString::fromStdString(message);

        QMessageBox box(icon, title, qmsg);
        box.exec();
    }

}

bool UserMessage::confirm(const std::string& message)
{
    bool ok = true;
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(message));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel)
    {
        ok=false;
    }
    return ok;
}


void UserMessage::debug(const std::string& message)
{
    UiLog().dbg() << message;
}

std::string UserMessage::toString(int v)
{
    return QString::number(v).toStdString();
}
