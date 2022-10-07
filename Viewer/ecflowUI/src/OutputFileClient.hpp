//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_OUTPUTFILECLIENT_HPP_
#define VIEWER_SRC_OUTPUTFILECLIENT_HPP_

#include "OutputClient.hpp"
#include "VDir.hpp"
#include "VFile.hpp"

class OutputVersionClient;

class OutputFileClient : public OutputClient
{
	Q_OBJECT

public:
    OutputFileClient(const std::string& host,const std::string& port,QObject *parent);

    VFile_ptr result() const;
    void clearResult();
    void getFile(const std::string& name);
    void getFile(const std::string& name, size_t deltaPos, unsigned int modTime, const std::string& checkSum);
    void setExpectedSize(qint64 v);
    int maxProgress() const;
    void setDir(VDir_ptr);

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err) override;
    void slotRead() override;
    void slotConnected() override;
    void slotVersionFinished();
    void slotVersionError(QString);

private:
	OutputFileClient(const OutputClient&);
	OutputFileClient& operator=(const OutputClient&);
    void estimateExpectedSize();
    bool parseResultHeader(char* buf, quint64& len);
    bool getHeaderValue(char* buf, quint64 len, int pos1, int& pos2, std::string& val);
    enum RequestType {GetRequest, GetFRequest, DeltaRequest, NoRequest};

    qint64 total_{0};
    qint64 expected_{0};
	VFile_ptr out_;
    VDir_ptr dir_;
    qint64 lastProgress_{0};
    bool readStarted_{false};
    const QString progressUnits_{"MB"};
    const qint64 progressChunk_{1024*1024};
    OutputVersionClient* versionClient_{nullptr};

    RequestType reqType_{NoRequest};
    size_t deltaPos_{0};
    unsigned int remoteModTime_{0};
    std::string  remoteCheckSum_;
};

class OutputVersionClient : public OutputClient
{
    Q_OBJECT

public:
    using OutputClient::OutputClient;
    OutputVersionClient(const OutputClient&) = delete;
    OutputVersionClient& operator=(const OutputClient&) = delete;
    void getVersion();
    int version() const {return version_;}

    enum VersionStatus {VersionNotFetched, VersionBeingFetched, VersionFailedToFetch, VersionFetched};
    VersionStatus versionStatus() const {return versionStatus_;}

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err) override;
    void slotRead() override;
    void slotConnected() override;

protected:
    void timeoutError() override;

private:
    void buildVersion();


    size_t dataSize_;
    static constexpr size_t maxDataSize_{64};
    char data_[maxDataSize_+1];
    int version_{0};
    VersionStatus versionStatus_{VersionNotFetched};
};

#endif /* VIEWER_SRC_OUTPUTFILECLIENT_HPP_ */
