//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputClient.hpp"

#include <QNetworkProxy>
#include <QTimer>
#include <QtGlobal>

#include "UiLog.hpp"

OutputClient::OutputClient(const std::string& host, const std::string& portStr, QObject* parent)
    : QObject(parent), soc_(nullptr), host_(host), portStr_(portStr), port_(19999), timeout_(3000) {
    if (!portStr_.empty())
        port_ = atoi(portStr.c_str());

    soc_ = new QTcpSocket(nullptr);

    soc_->setProxy(QNetworkProxy::NoProxy);

    connect(soc_, SIGNAL(readyRead()), this, SLOT(slotRead()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(soc_,
            SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
#else
    connect(soc_,
            SIGNAL(error(QAbstractSocket::SocketError)),
#endif
            this,
            SLOT(slotError(QAbstractSocket::SocketError)));

    connect(soc_, SIGNAL(connected()), this, SLOT(slotConnected()));

    if (char* timeoutStr = getenv("ECFLOWVIEW_LOGTIMEOUT"))
        timeout_ = atoi(timeoutStr) * 1000;
}

OutputClient::~OutputClient() {
    soc_->abort();
}

void OutputClient::connectToHost(std::string host, int port) {
    stopper_.start();
    soc_->abort();
    if (host != "localhost") {
        soc_->connectToHost(QString::fromStdString(host), port);
    }
    else {
        // On macOS using the string "localhost" did not work
        soc_->connectToHost(QHostAddress::LocalHost, port);
    }
    // We cannot change the timeout through the qt api so we need this hack.
    QTimer::singleShot(timeout_, this, SLOT(slotCheckTimeout()));
}

void OutputClient::slotCheckTimeout() {
    if (soc_->state() == QAbstractSocket::HostLookupState || soc_->state() == QAbstractSocket::ConnectingState) {
        soc_->abort();
        timeoutError();
        Q_EMIT error("Timeout error");
    }
}

std::string OutputClient::longName() const {
    return host_ + "@" + portStr_;
}
