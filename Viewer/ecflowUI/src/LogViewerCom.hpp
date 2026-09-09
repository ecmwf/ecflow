/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_LogViewerCom_HPP
#define ecflow_viewer_LogViewerCom_HPP

#ifdef ECFLOW_LOGVIEW

    #include <sstream>
    #include <string>
    #include <vector>

    #include <QDateTime>
    #include <QObject>

class ServerHandler;
class QLocalSocket;

class LogViewerCom : public QObject {
public:
    LogViewerCom();

    void addToApp(ServerHandler*);
    void closeApp();

protected:
    void start(QStringList);

    QString program_;
    QString logViewerId_;
};

#endif // ECFLOW_LOGVIEW

#endif /* ecflow_viewer_LogViewerCom_HPP */
