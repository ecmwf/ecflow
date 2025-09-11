/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "OutputDirClient.hpp"

#include <memory>

#include "UiLog.hpp"
#include "ecflow/core/Filesystem.hpp"

#define _UI_OUTPUTDIRCLIENT_DEBUG

OutputDirClient::OutputDirClient(const std::string& host, const std::string& portStr, QObject* parent)
    : OutputClient(host, portStr, parent) {
}

VDir_ptr OutputDirClient::result() const {
    return dir_;
}

void OutputDirClient::slotCheckTimeout() {
    if (soc_->state() == QAbstractSocket::HostLookupState || soc_->state() == QAbstractSocket::ConnectingState) {
        soc_->abort();
        if (dir_) {
            dir_.reset();
        }

        Q_EMIT error("Timeout error");
    }
}

void OutputDirClient::slotConnected() {
#ifdef _UI_OUTPUTDIRCLIENT_DEBUG
    UiLog().dbg() << "OutputDirClient::slotConnected() connected to " << soc_->peerName();
#endif
    soc_->write("list ", 5);
    soc_->write(remoteFile_.c_str(), remoteFile_.size());
    soc_->write("\n", 1);
}

void OutputDirClient::slotError(QAbstractSocket::SocketError err) {
#ifdef _UI_OUTPUTDIRCLIENT_DEBUG
    UiLog().dbg() << "OutputDirClient::slotError --> " << soc_->errorString();
#endif
    switch (err) {
        // The logserver does not notify us if the file trasfer finish. We simply get this error.
        case QAbstractSocket::RemoteHostClosedError:

#ifdef _UI_OUTPUTDIRCLIENT_DEBUG
            UiLog().dbg() << "   RemoteHostClosedError ";
#endif
            // If no data was transferred we think it is a real error.
            if (data_.isEmpty()) {
#ifdef _UI_OUTPUTDIRCLIENT_DEBUG
                UiLog().dbg() << "   --> data is empty: file transfer failed";
#endif
                break;
            }
            // If there is some data we think the transfer succeeded.
            else {
#ifdef _UI_OUTPUTDIRCLIENT_DEBUG
                UiLog().dbg() << "   --> has data: file transfer succeeded";
#endif
                if (dir_) {
                    parseData();
                }

                Q_EMIT finished();

                if (dir_) {
                    dir_.reset();
                }

                return;
            }

            break;
        case QAbstractSocket::UnknownSocketError:
            if (soc_->state() != QAbstractSocket::ConnectedState) {
                break;
            }
            break;
        default:
            break;
    }

    soc_->abort();
    if (dir_) {
        dir_.reset();
    }
    Q_EMIT error(soc_->errorString());
}

void OutputDirClient::getDir(const std::string& name) {
    fs::path fp(name);
    std::string dirName = fp.parent_path().string();

    remoteFile_ = name;
    dir_.reset();
    dir_ = std::make_shared<VDir>(dirName);
    data_.clear();

    // indicates the source of the files
    dir_->where(host_ + "@" + portStr_);

    connectToHost(host_, port_);
}

void OutputDirClient::slotRead() {
    const qint64 size = 64 * 1024;
    char buf[size + 1];
    quint64 len = 0;

    while ((len = soc_->read(buf, size)) > 0) {
        data_.append(buf, len);
    }
}

void OutputDirClient::parseData() {
    if (!dir_) {
        return;
    }

    QTextStream in(data_);
    while (!in.atEnd()) {
        int mode = 0, uid = 0, gid = 0;
        unsigned int size  = 0;
        unsigned int atime = 0, mtime = 0, ctime = 0;
        QString name;

        in >> mode >> uid >> gid >> size >> atime >> mtime >> ctime >> name;

        if (!name.isEmpty()) {
            fs::path p(name.toStdString());
            std::string fileDirName = p.parent_path().string();
            std::string fileName    = p.filename().string();

            // Adjust the path in the dir
            if (dir_->path() != fileDirName) {
                dir_->path(fileDirName, false);
            }

            dir_->addItem(fileName, size, mtime);
        }
    }
}
