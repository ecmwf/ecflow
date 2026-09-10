/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OutputDirClient_HPP
#define ecflow_viewer_OutputDirClient_HPP

#include <QByteArray>

#include "OutputClient.hpp"
#include "VDir.hpp"

class OutputDirClient : public OutputClient {
    Q_OBJECT

public:
    OutputDirClient(const std::string& host, const std::string& port, QObject* parent);

    VDir_ptr result() const;
    void getDir(const std::string& name);

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err) override;
    void slotRead() override;
    void slotConnected() override;
    void slotCheckTimeout();

private:
    OutputDirClient(const OutputDirClient&);
    OutputDirClient& operator=(const OutputDirClient&);

    void parseData();

    VDir_ptr dir_;
    QByteArray data_;
};

#endif /* ecflow_viewer_OutputDirClient_HPP */
