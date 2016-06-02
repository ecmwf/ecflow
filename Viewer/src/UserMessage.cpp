
//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <iostream>

#include <QMessageBox>

#include "UserMessage.hpp"

bool UserMessage::echoToCout_ = true;  // XXX should be false to start with

UserMessage::UserMessage()
{
}

void UserMessage::message(MessageType type, bool popup, const std::string& message)
{

    if (echoToCout_)
    {
        switch (type)
        {
            case INFO:  std::cout << "INFO  : "; break;
            case WARN:  std::cout << "WARN  : "; break;
            case ERROR: std::cout << "ERROR : "; break;
            case DBG:   std::cout << "DEBUG : "; break;
            default:    std::cout << "      : "; break;
        }

        std::cout << message << std::endl;
    }

    if (popup)
    {
        QMessageBox::Icon icon;
        switch (type)
        {
            case INFO:  icon = QMessageBox::Information; break;
            case WARN:  icon = QMessageBox::Warning;     break;
            case ERROR: icon = QMessageBox::Critical;    break;
            case DBG:   icon = QMessageBox::NoIcon;      break;
            default:    icon = QMessageBox::NoIcon;      break;
        }
        QString title("Ecflowview");
        QString qmsg = QString::fromStdString(message);

        QMessageBox box(icon, title, qmsg);
        box.exec();
    }

}

void UserMessage::debug(const std::string& message)
{
        std::cout << "DEBUG : " << message << std::endl;
}

void UserMessage::qdebug(QString message)
{
        std::cout << "DEBUG : " << message.toStdString() << std::endl;
}

std::string UserMessage::toString(int v)
{
    return QString::number(v).toStdString();
}
