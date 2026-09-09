/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_UserMessage_HPP
#define ecflow_viewer_UserMessage_HPP

#include <string>

#include <QDebug>
#include <QString>

class UserMessage {
    // Q_OBJECT
public:
    UserMessage();

    enum MessageType { INFO, WARN, ERROR, DBG }; // note: cannot have DEBUG because of possible -DDEBUG in cpp!

    static void setEchoToCout(bool toggle) { echoToCout_ = toggle; }
    static void message(MessageType type, bool popup, const std::string& message);
    static bool confirm(const std::string& message);

    static void debug(const std::string& message);
    static std::string toString(int);

private:
    static bool echoToCout_;
};

#endif /* ecflow_viewer_UserMessage_HPP */
